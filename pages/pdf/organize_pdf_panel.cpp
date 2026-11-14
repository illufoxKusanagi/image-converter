#include "organize_pdf_panel.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QAbstractItemView>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QListWidgetItem>
#include <QPdfDocument>
#include <QPixmap>
#include <QPushButton>
#include <QSize>
#include <QSizeF>
#include <QTransform>
#include <QVBoxLayout>
#include <QtGlobal>
#include <utility>

OrganizePdfPanel::OrganizePdfPanel(QWidget *parent) : QWidget(parent) {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(6);

  auto *toolbarLayout = new QHBoxLayout();
  toolbarLayout->setContentsMargins(0, 0, 0, 0);
  toolbarLayout->setSpacing(6);

  m_hintLabel = new QLabel(
      "Pages (drag or use buttons to reorder, rotate, or delete):", this);
  m_loadBtn = new ui::Button("Load Pages from PDF", ui::ButtonVariant::Secondary,
                             ui::ButtonSize::Small, this);
  m_rotLeftBtn = new ui::Button("Rotate Left (-90°)", ui::ButtonVariant::Outline,
                                ui::ButtonSize::Small, this);
  m_rotRightBtn =
      new ui::Button("Rotate Right (+90°)", ui::ButtonVariant::Outline,
                     ui::ButtonSize::Small, this);
  m_rot180Btn = new ui::Button("Rotate 180°", ui::ButtonVariant::Outline,
                               ui::ButtonSize::Small, this);
  m_moveUpBtn = new ui::Button("Move Up", ui::ButtonVariant::Outline,
                               ui::ButtonSize::Small, this);
  m_moveDownBtn = new ui::Button("Move Down", ui::ButtonVariant::Outline,
                                 ui::ButtonSize::Small, this);
  m_deleteBtn = new ui::Button("Delete Page", ui::ButtonVariant::Destructive,
                               ui::ButtonSize::Small, this);
  m_resetBtn = new ui::Button("Reset", ui::ButtonVariant::Ghost,
                              ui::ButtonSize::Small, this);

  for (ui::Button *btn : {m_loadBtn, m_resetBtn}) {
    connect(btn, &QPushButton::clicked, this,
            &OrganizePdfPanel::reloadRequested);
  }

  const std::pair<ui::Button *, int> rotateBindings[] = {
      {m_rotLeftBtn, -90},
      {m_rotRightBtn, 90},
      {m_rot180Btn, 180},
  };
  for (const auto &binding : rotateBindings) {
    const int delta = binding.second;
    connect(binding.first, &QPushButton::clicked, this,
            [this, delta]() { rotateSelectedPages(delta); });
  }

  const std::pair<ui::Button *, int> moveBindings[] = {
      {m_moveUpBtn, -1},
      {m_moveDownBtn, 1},
  };
  for (const auto &binding : moveBindings) {
    const int direction = binding.second;
    connect(binding.first, &QPushButton::clicked, this,
            [this, direction]() { moveSelectedPage(direction); });
  }

  connect(m_deleteBtn, &QPushButton::clicked, this,
          &OrganizePdfPanel::deleteSelectedPages);

  toolbarLayout->addWidget(m_loadBtn);
  toolbarLayout->addWidget(m_rotLeftBtn);
  toolbarLayout->addWidget(m_rotRightBtn);
  toolbarLayout->addWidget(m_rot180Btn);
  toolbarLayout->addWidget(m_moveUpBtn);
  toolbarLayout->addWidget(m_moveDownBtn);
  toolbarLayout->addWidget(m_deleteBtn);
  toolbarLayout->addWidget(m_resetBtn);
  toolbarLayout->addStretch(1);

  m_pageList = new QListWidget(this);
  m_pageList->setViewMode(QListWidget::ListMode);
  m_pageList->setIconSize(QSize(40, 56));
  m_pageList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_pageList->setDragDropMode(QAbstractItemView::InternalMove);
  m_pageList->setDefaultDropAction(Qt::MoveAction);
  m_pageList->setFixedHeight(140);

  mainLayout->addWidget(m_hintLabel);
  mainLayout->addLayout(toolbarLayout);
  mainLayout->addWidget(m_pageList);

  applyThemeStyles();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &OrganizePdfPanel::applyThemeStyles);
}

void OrganizePdfPanel::loadFromPaths(const QStringList &paths,
                                     bool silentIfEmpty) {
  if (!m_pageList) {
    return;
  }
  QWidget *toastParent = window() ? window() : this;

  if (paths.isEmpty()) {
    m_pageList->clear();
    m_sourcePath.clear();
    if (!silentIfEmpty) {
      ui::Toast::show(toastParent, "No PDF Selected",
                      "Please add a PDF file in the drop zone first.",
                      ui::ToastVariant::Warning);
    }
    return;
  }

  const QString sourcePath = paths.first();
  QPdfDocument doc;
  const auto err = doc.load(sourcePath);
  if (err != QPdfDocument::Error::None || doc.pageCount() <= 0) {
    m_pageList->clear();
    m_sourcePath.clear();
    ui::Toast::show(
        toastParent, "Cannot Load PDF Pages",
        QString("Could not open '%1' (damaged or password-protected).")
            .arg(QFileInfo(sourcePath).fileName()),
        ui::ToastVariant::Destructive);
    return;
  }

  m_pageList->clear();
  m_sourcePath = sourcePath;

  for (int i = 0; i < doc.pageCount(); ++i) {
    const QSizeF ptSize = doc.pagePointSize(i);
    const QSize thumbSize(80, 112);
    const QImage thumb = pdfutil::renderPdfPageOnWhite(doc, i, thumbSize);

    auto *item = new QListWidgetItem(m_pageList);
    item->setData(Qt::UserRole, i);
    item->setData(Qt::UserRole + 1, 0);
    item->setData(Qt::UserRole + 2, ptSize);
    item->setData(Qt::UserRole + 3, thumb);
    updateItemDisplay(item);
  }

  if (m_pageList->count() > 0) {
    m_pageList->setCurrentRow(0);
  }
}

QVector<OrganizeController::PageOp> OrganizePdfPanel::pageOps() const {
  QVector<OrganizeController::PageOp> ops;
  if (!m_pageList) {
    return ops;
  }
  ops.reserve(m_pageList->count());
  for (int i = 0; i < m_pageList->count(); ++i) {
    const QListWidgetItem *item = m_pageList->item(i);
    if (!item) {
      continue;
    }
    OrganizeController::PageOp op;
    op.originalPageIndex = item->data(Qt::UserRole).toInt();
    op.rotationDegrees = item->data(Qt::UserRole + 1).toInt();
    ops.append(op);
  }
  return ops;
}

QString OrganizePdfPanel::sourcePath() const { return m_sourcePath; }

int OrganizePdfPanel::pageCount() const {
  return m_pageList ? m_pageList->count() : 0;
}

void OrganizePdfPanel::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  if (m_hintLabel) {
    m_hintLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_hintLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.foreground)));
  }

  if (m_pageList) {
    const QString listStyle =
        QString("QListWidget {"
                "  background-color: %1;"
                "  color: %2;"
                "  border: 1px solid %3;"
                "  border-radius: %4px;"
                "  padding: 4px;"
                "  outline: none;"
                "}"
                "QListWidget::item {"
                "  padding: 5px 8px;"
                "  border-radius: %5px;"
                "}"
                "QListWidget::item:selected {"
                "  background-color: %6;"
                "  color: %2;"
                "}"
                "QListWidget::item:hover {"
                "  background-color: %7;"
                "}")
            .arg(ui::StyleHelper::toHexString(c.card))
            .arg(ui::StyleHelper::toHexString(c.foreground))
            .arg(ui::StyleHelper::toHexString(c.border))
            .arg(r.md)
            .arg(r.sm)
            .arg(ui::StyleHelper::toHexString(c.accent))
            .arg(ui::StyleHelper::toHexString(c.secondary));

    m_pageList->setFont(t.font(t.sizeSm));
    m_pageList->setStyleSheet(listStyle);
  }
}

void OrganizePdfPanel::rotateSelectedPages(int deltaDegrees) {
  if (!m_pageList) {
    return;
  }
  const QList<QListWidgetItem *> selected = m_pageList->selectedItems();
  if (selected.isEmpty()) {
    ui::Toast::show(window() ? window() : this, "No Page Selected",
                    "Select one or more pages in the list to rotate.",
                    ui::ToastVariant::Warning);
    return;
  }
  for (QListWidgetItem *item : selected) {
    int rot = (item->data(Qt::UserRole + 1).toInt() + deltaDegrees) % 360;
    if (rot < 0) {
      rot += 360;
    }
    item->setData(Qt::UserRole + 1, rot);
    updateItemDisplay(item);
  }
}

void OrganizePdfPanel::moveSelectedPage(int direction) {
  if (!m_pageList || direction == 0) {
    return;
  }
  const int row = m_pageList->currentRow();
  const int targetRow = row + direction;
  if (row < 0 || targetRow < 0 || targetRow >= m_pageList->count()) {
    return;
  }
  QListWidgetItem *item = m_pageList->takeItem(row);
  if (!item) {
    return;
  }
  m_pageList->insertItem(targetRow, item);
  m_pageList->setCurrentRow(targetRow);
}

void OrganizePdfPanel::deleteSelectedPages() {
  if (!m_pageList) {
    return;
  }
  QWidget *toastParent = window() ? window() : this;
  const QList<QListWidgetItem *> selected = m_pageList->selectedItems();
  if (selected.isEmpty()) {
    ui::Toast::show(toastParent, "No Page Selected",
                    "Select one or more pages in the list to delete.",
                    ui::ToastVariant::Warning);
    return;
  }
  if (m_pageList->count() - selected.size() < 1) {
    ui::Toast::show(toastParent, "Cannot Delete All Pages",
                    "At least one page must remain in the document.",
                    ui::ToastVariant::Warning);
    return;
  }
  for (QListWidgetItem *item : selected) {
    delete m_pageList->takeItem(m_pageList->row(item));
  }
}

void OrganizePdfPanel::updateItemDisplay(QListWidgetItem *item) const {
  if (!item) {
    return;
  }
  const int origIndex = item->data(Qt::UserRole).toInt();
  const int rot = item->data(Qt::UserRole + 1).toInt();
  const QSizeF ptSize = item->data(Qt::UserRole + 2).toSizeF();
  const QImage baseThumb = item->data(Qt::UserRole + 3).value<QImage>();

  if (!baseThumb.isNull()) {
    const QImage rotatedThumb =
        (rot == 0) ? baseThumb
                   : baseThumb.transformed(QTransform().rotate(rot),
                                           Qt::SmoothTransformation);
    item->setIcon(QIcon(QPixmap::fromImage(rotatedThumb)));
  }

  item->setText(QString("Page %1 (%2 × %3 pt) — Rotation: %4°")
                    .arg(origIndex + 1)
                    .arg(qRound(ptSize.width()))
                    .arg(qRound(ptSize.height()))
                    .arg(rot));
}
