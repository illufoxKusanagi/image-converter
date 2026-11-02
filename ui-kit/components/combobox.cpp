#include "combobox.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include "popover.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPalette>
#include <QPointer>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QToolButton>
#include <QVBoxLayout>

namespace ui {

Combobox::Combobox(QWidget *parent) : Combobox(QStringList(), parent) {}

Combobox::Combobox(const QStringList &items, QWidget *parent)
    : QWidget(parent), m_layout(new QHBoxLayout(this)),
      m_lineEdit(new QLineEdit(this)), m_button(new QToolButton(this)),
      m_popup(new Popover(this)), m_searchEdit(new QLineEdit(m_popup)),
      m_list(new QListView(m_popup)),
      m_emptyLabel(new QLabel(tr("No results found"), m_popup)),
      m_model(new QStandardItemModel(this)),
      m_proxy(new QSortFilterProxyModel(this)) {
  setAttribute(Qt::WA_StyledBackground, true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(m_lineEdit);
  setAccessibleName(tr("Combobox"));
  setAccessibleDescription(tr(
      "Type to search, use arrow keys to choose, and press Enter to select."));

  m_lineEdit->setObjectName(QStringLiteral("uiComboboxEditor"));
  m_lineEdit->setFrame(false);
  m_lineEdit->setPlaceholderText(tr("Select an item"));
  m_lineEdit->setAccessibleName(tr("Selected item"));
  m_lineEdit->setAccessibleDescription(accessibleDescription());
  m_lineEdit->installEventFilter(this);

  m_button->setObjectName(QStringLiteral("uiComboboxButton"));
  m_button->setArrowType(Qt::DownArrow);
  m_button->setFocusPolicy(Qt::NoFocus);
  m_button->setAccessibleName(tr("Show choices"));
  m_button->setToolTip(tr("Show choices"));
  m_layout->addWidget(m_lineEdit, 1);
  m_layout->addWidget(m_button);

  m_searchEdit->setObjectName(QStringLiteral("uiComboboxSearch"));
  m_searchEdit->setPlaceholderText(tr("Search items..."));
  m_searchEdit->setAccessibleName(tr("Search items"));
  m_searchEdit->setAccessibleDescription(accessibleDescription());
  m_searchEdit->installEventFilter(this);
  m_proxy->setSourceModel(m_model);
  m_proxy->setFilterKeyColumn(0);
  m_proxy->setFilterRole(Qt::DisplayRole);
  m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  m_list->setObjectName(QStringLiteral("uiComboboxResults"));
  m_list->setModel(m_proxy);
  m_list->setFrameShape(QFrame::NoFrame);
  m_list->setSelectionMode(QAbstractItemView::SingleSelection);
  m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_list->setFocusPolicy(Qt::NoFocus);
  m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_list->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  m_list->setTextElideMode(Qt::ElideRight);
  m_list->setUniformItemSizes(true);
  m_list->setMinimumSize(0, 0);
  m_list->setAccessibleName(tr("Matching items"));
  m_list->installEventFilter(this);
  m_emptyLabel->setObjectName(QStringLiteral("uiComboboxEmpty"));
  m_emptyLabel->setAlignment(Qt::AlignCenter);
  m_emptyLabel->setWordWrap(true);
  m_popup->setAccessibleName(tr("Combobox choices"));
  m_popup->contentLayout()->addWidget(m_searchEdit);
  m_popup->contentLayout()->addWidget(m_list);
  m_popup->contentLayout()->addWidget(m_emptyLabel);

  connect(m_lineEdit, &QLineEdit::textEdited, this, &Combobox::showPopup);
  connect(m_searchEdit, &QLineEdit::textEdited, this, &Combobox::updateFilter);
  connect(m_list, &QListView::clicked, this, &Combobox::commitIndex);
  connect(m_button, &QToolButton::clicked, this, [this]() {
    if (m_popup->isVisible()) {
      m_popup->closePopover();
    } else {
      showPopup();
    }
  });
  connect(m_popup, &Popover::closed, this, [this]() {
    restoreCommittedText();
    applyThemeStyles();
  });
  connect(qApp, &QApplication::focusChanged, this,
          [this](QWidget *, QWidget *focused) {
            if (!m_openingPopup && m_popup->isVisible() && focused &&
                focused != m_popup && !m_popup->isAncestorOf(focused)) {
              m_popup->closePopover();
            }
          });
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Combobox::applyThemeStyles);
  setItems(items);
  applyThemeStyles();
}

Combobox::~Combobox() {
  disconnect(m_popup, nullptr, this, nullptr);
  m_popup->closePopover();
}

void Combobox::setItems(const QStringList &items) {
  const int oldIndex = m_currentIndex;
  const QString oldText = currentText();
  m_popup->closePopover();
  m_currentIndex = -1;
  m_model->clear();
  for (const QString &text : items) {
    auto *item = new QStandardItem(text);
    item->setEditable(false);
    m_model->appendRow(item);
  }
  restoreCommittedText();
  updateResults();
  notifySelectionChanged(oldIndex, oldText);
}

void Combobox::addItem(const QString &text, const QVariant &data) {
  auto *item = new QStandardItem(text);
  item->setEditable(false);
  item->setData(data, Qt::UserRole);
  m_model->appendRow(item);
  if (m_popup->isVisible()) {
    updateFilter(m_searchEdit->text());
  } else {
    updateResults();
  }
}

int Combobox::currentIndex() const { return m_currentIndex; }

void Combobox::setCurrentIndex(int index) {
  if (index < 0 || index >= m_model->rowCount()) {
    index = -1;
  }
  if (index >= 0 && !isSelectable(m_model->index(index, 0))) {
    return;
  }
  const int oldIndex = m_currentIndex;
  const QString oldText = currentText();
  m_currentIndex = index;
  m_popup->closePopover();
  restoreCommittedText();
  notifySelectionChanged(oldIndex, oldText);
}

QString Combobox::currentText() const {
  return m_currentIndex >= 0 ? m_model->index(m_currentIndex, 0)
                                   .data(Qt::DisplayRole)
                                   .toString()
                             : QString();
}

QVariant Combobox::currentData() const {
  return m_currentIndex >= 0
             ? m_model->index(m_currentIndex, 0).data(Qt::UserRole)
             : QVariant();
}

void Combobox::setPlaceholderText(const QString &placeholder) {
  m_lineEdit->setPlaceholderText(placeholder);
}

bool Combobox::hasError() const { return m_hasError; }

void Combobox::setError(bool hasError) {
  if (m_hasError != hasError) {
    m_hasError = hasError;
    applyThemeStyles();
  }
}

void Combobox::showPopup(const QString &query) {
  if (!isEnabled() || !isVisible()) {
    restoreCommittedText();
    return;
  }
  m_openingPopup = true;
  {
    const QSignalBlocker blocker(m_searchEdit);
    m_searchEdit->setText(query);
    m_searchEdit->setCursorPosition(query.size());
  }
  updateFilter(query);
  m_popup->openFor(this);
  m_openingPopup = false;
  if (!m_popup->isVisible()) {
    restoreCommittedText();
  }
  applyThemeStyles();
}

void Combobox::updateFilter(const QString &query) {
  m_proxy->setFilterFixedString(query);
  m_list->clearSelection();
  m_list->setCurrentIndex(QModelIndex());
  {
    const QSignalBlocker blocker(m_lineEdit);
    m_lineEdit->setText(query);
  }
  updateResults();
  if (m_popup->isVisible()) {
    m_popup->openFor(this);
  }
}

void Combobox::updateResults() {
  const int count = m_proxy->rowCount();
  const auto &s = Theme::instance().spacing();
  const int rowHeight = m_list->fontMetrics().height() + 2 * s.space2;
  m_list->setMaximumHeight(qBound(1, count, 8) * rowHeight);
  m_list->setVisible(count > 0);
  m_emptyLabel->setVisible(count == 0);
}

void Combobox::restoreCommittedText() {
  const QSignalBlocker editorBlocker(m_lineEdit);
  const QSignalBlocker searchBlocker(m_searchEdit);
  m_lineEdit->setText(currentText());
  m_searchEdit->clear();
  m_proxy->setFilterFixedString(QString());
  m_list->clearSelection();
  m_list->setCurrentIndex(QModelIndex());
  updateResults();
}

bool Combobox::isSelectable(const QModelIndex &index) const {
  return index.isValid() && (index.flags() & Qt::ItemIsEnabled) &&
         (index.flags() & Qt::ItemIsSelectable);
}

void Combobox::moveHighlight(int direction) {
  const int count = m_proxy->rowCount();
  int row = m_list->currentIndex().isValid() ? m_list->currentIndex().row()
                                             : (direction > 0 ? -1 : count);
  for (row += direction; row >= 0 && row < count; row += direction) {
    const QModelIndex index = m_proxy->index(row, 0);
    if (isSelectable(index)) {
      m_list->setCurrentIndex(index);
      m_list->scrollTo(index);
      return;
    }
  }
}

void Combobox::commitIndex(const QModelIndex &proxyIndex) {
  if (!isEnabled() || !isSelectable(proxyIndex)) {
    return;
  }
  const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
  if (isSelectable(sourceIndex)) {
    setCurrentIndex(sourceIndex.row());
  }
}

void Combobox::notifySelectionChanged(int oldIndex, const QString &oldText) {
  const int index = m_currentIndex;
  const QString text = currentText();
  QPointer<Combobox> guard(this);
  if (index != oldIndex) {
    emit currentIndexChanged(index);
  }
  if (guard && m_currentIndex == index && currentText() == text &&
      text != oldText) {
    emit currentTextChanged(text);
  }
}

bool Combobox::eventFilter(QObject *watched, QEvent *event) {
  const bool editor = watched == m_lineEdit || watched == m_searchEdit;
  if (editor &&
      (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut)) {
    if (watched == m_lineEdit && event->type() == QEvent::FocusOut &&
        !m_openingPopup && !m_popup->isVisible()) {
      restoreCommittedText();
    }
    applyThemeStyles();
  }
  if (editor || watched == m_list) {
    if (event->type() == QEvent::ShortcutOverride) {
      auto *key = static_cast<QKeyEvent *>(event);
      if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up ||
          key->key() == Qt::Key_F4 ||
          (m_popup->isVisible() &&
           (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter ||
            key->key() == Qt::Key_Escape || key->key() == Qt::Key_Tab ||
            key->key() == Qt::Key_Backtab))) {
        event->accept();
        return true;
      }
    } else if (event->type() == QEvent::KeyPress && isEnabled()) {
      auto *key = static_cast<QKeyEvent *>(event);
      switch (key->key()) {
      case Qt::Key_Down:
      case Qt::Key_Up:
        if (!m_popup->isVisible()) {
          showPopup();
        }
        moveHighlight(key->key() == Qt::Key_Down ? 1 : -1);
        return true;
      case Qt::Key_Return:
      case Qt::Key_Enter:
        if (m_popup->isVisible()) {
          if (!m_list->currentIndex().isValid()) {
            moveHighlight(1);
          }
          commitIndex(m_list->currentIndex());
          return true;
        }
        break;
      case Qt::Key_Escape:
        if (m_popup->isVisible()) {
          m_popup->closePopover();
          return true;
        }
        break;
      case Qt::Key_F4:
        if (m_popup->isVisible()) {
          m_popup->closePopover();
        } else {
          showPopup();
        }
        return true;
      case Qt::Key_Tab:
      case Qt::Key_Backtab:
        if (m_popup->isVisible()) {
          const bool forward = key->key() == Qt::Key_Tab &&
                               !(key->modifiers() & Qt::ShiftModifier);
          m_popup->closePopover();
          m_lineEdit->setFocus(forward ? Qt::TabFocusReason
                                       : Qt::BacktabFocusReason);
          focusNextPrevChild(forward);
          return true;
        }
        break;
      default:
        break;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

void Combobox::changeEvent(QEvent *event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::EnabledChange) {
    if (!isEnabled()) {
      m_popup->closePopover();
      restoreCommittedText();
    }
    applyThemeStyles();
  }
}

void Combobox::hideEvent(QHideEvent *event) {
  m_popup->closePopover();
  restoreCommittedText();
  QWidget::hideEvent(event);
}

void Combobox::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &r = theme.radius();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();
  const bool focused = m_lineEdit->hasFocus() || m_searchEdit->hasFocus() ||
                       m_popup->isVisible();
  const QColor border =
      m_hasError ? c.destructive : (focused ? c.ring : c.border);
  const QColor hoverBorder = m_hasError ? c.destructive : c.ring;
  const QFont controlFont = t.font(t.sizeBase);
  setFont(controlFont);
  m_lineEdit->setFont(controlFont);
  m_searchEdit->setFont(controlFont);
  m_list->setFont(controlFont);
  m_emptyLabel->setFont(t.font(t.sizeSm));
  setMinimumHeight(t.sizeBase + s.space6);
  m_layout->setContentsMargins(s.space3, s.space1, s.space1, s.space1);
  m_layout->setSpacing(s.space1);
  m_button->setFixedSize(s.space8, s.space8);

  QPalette editorPalette = m_lineEdit->palette();
  editorPalette.setColor(QPalette::Text, c.foreground);
  editorPalette.setColor(QPalette::PlaceholderText, c.mutedForeground);
  editorPalette.setColor(QPalette::Highlight, c.primary);
  editorPalette.setColor(QPalette::HighlightedText, c.primaryForeground);
  editorPalette.setColor(QPalette::Disabled, QPalette::Text, c.mutedForeground);
  m_lineEdit->setPalette(editorPalette);
  editorPalette.setColor(QPalette::Text, c.popoverForeground);
  m_searchEdit->setPalette(editorPalette);

  setStyleSheet(
      QStringLiteral("ui--Combobox {"
                     "  background-color: %1;"
                     "  border: 1px solid %2;"
                     "  border-radius: %3px;"
                     "}"
                     "ui--Combobox:hover { border-color: %4; }"
                     "ui--Combobox:disabled { background-color: %5; }")
          .arg(StyleHelper::toHexString(c.background))
          .arg(StyleHelper::toHexString(border))
          .arg(r.md)
          .arg(StyleHelper::toHexString(hoverBorder))
          .arg(StyleHelper::toHexString(c.muted)));
  m_lineEdit->setStyleSheet(QStringLiteral(
      "QLineEdit#uiComboboxEditor { background: transparent; border: none; }"));
  m_searchEdit->setStyleSheet(
      QStringLiteral("QLineEdit#uiComboboxSearch {"
                     "  background-color: %1;"
                     "  border: 1px solid %2;"
                     "  border-radius: %3px;"
                     "  padding: %4px %5px;"
                     "}"
                     "QLineEdit#uiComboboxSearch:focus { border-color: %6; }")
          .arg(StyleHelper::toHexString(c.popover))
          .arg(StyleHelper::toHexString(c.input))
          .arg(r.sm)
          .arg(s.space1)
          .arg(s.space2)
          .arg(StyleHelper::toHexString(c.ring)));
  m_button->setStyleSheet(
      QStringLiteral(
          "QToolButton#uiComboboxButton {"
          "  background: transparent; border: none; color: %1; border-radius: "
          "%2px;"
          "}"
          "QToolButton#uiComboboxButton:hover { background-color: %3; }"
          "QToolButton#uiComboboxButton:disabled { color: %4; }")
          .arg(StyleHelper::toHexString(c.foreground))
          .arg(r.sm)
          .arg(StyleHelper::toHexString(c.accent))
          .arg(StyleHelper::toHexString(c.mutedForeground)));
  m_list->setStyleSheet(
      QStringLiteral(
          "QListView#uiComboboxResults {"
          "  background-color: %1; color: %2; border: none;"
          "}"
          "QListView#uiComboboxResults::item {"
          "  padding: %3px %4px; border-radius: %5px;"
          "}"
          "QListView#uiComboboxResults::item:selected { background-color: %6; "
          "color: %7; }"
          "QListView#uiComboboxResults::item:disabled { color: %8; }")
          .arg(StyleHelper::toHexString(c.popover))
          .arg(StyleHelper::toHexString(c.popoverForeground))
          .arg(s.space2)
          .arg(s.space3)
          .arg(r.sm)
          .arg(StyleHelper::toHexString(c.accent))
          .arg(StyleHelper::toHexString(c.accentForeground))
          .arg(StyleHelper::toHexString(c.mutedForeground)));
  m_emptyLabel->setStyleSheet(
      QStringLiteral("QLabel#uiComboboxEmpty { color: %1; padding: %2px; }")
          .arg(StyleHelper::toHexString(c.mutedForeground))
          .arg(s.space3));
  updateResults();
  if (m_popup->isVisible()) {
    m_popup->openFor(this);
  }
}

} // namespace ui
