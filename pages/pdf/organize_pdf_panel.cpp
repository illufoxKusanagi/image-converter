#include "organize_pdf_panel.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/scroll_area.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/i18n.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include <QAbstractItemView>
#include <QDir>
#include <QFileDialog>
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
      tr("Pages (drag or use buttons to reorder, rotate, or delete):"), this);
  m_loadBtn = new ui::Button(tr("Load Pages from PDF"),
                             ui::ButtonVariant::Secondary,
                             ui::ButtonSize::Small, this);
  m_rotLeftBtn = new ui::Button(tr("Rotate Left (-90°)"),
                                ui::ButtonVariant::Outline,
                                ui::ButtonSize::Small, this);
  m_rotRightBtn =
      new ui::Button(tr("Rotate Right (+90°)"), ui::ButtonVariant::Outline,
                     ui::ButtonSize::Small, this);
  m_rot180Btn = new ui::Button(tr("Rotate 180°"), ui::ButtonVariant::Outline,
                               ui::ButtonSize::Small, this);
  m_moveUpBtn = new ui::Button(tr("Move Up"), ui::ButtonVariant::Outline,
                               ui::ButtonSize::Small, this);
  m_moveDownBtn = new ui::Button(tr("Move Down"), ui::ButtonVariant::Outline,
                                 ui::ButtonSize::Small, this);
  m_deleteBtn = new ui::Button(tr("Delete Page"), ui::ButtonVariant::Destructive,
                               ui::ButtonSize::Small, this);
  m_resetBtn = new ui::Button(tr("Reset"), ui::ButtonVariant::Ghost,
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
  m_pageList->setMinimumHeight(140);
  m_pageList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  mainLayout->addWidget(m_hintLabel);
  mainLayout->addLayout(toolbarLayout);
  mainLayout->addWidget(m_pageList, 1);

  m_controller = new OrganizeController(this);
  connect(m_controller, &OrganizeController::finished, this,
          &OrganizePdfPanel::onOrganizeFinished, Qt::QueuedConnection);

  applyThemeStyles();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &OrganizePdfPanel::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &OrganizePdfPanel::retranslateUi);
}

void OrganizePdfPanel::retranslateUi() {
  if (m_hintLabel) {
    m_hintLabel->setText(
        tr("Pages (drag or use buttons to reorder, rotate, or delete):"));
  }
  if (m_loadBtn) {
    m_loadBtn->setText(tr("Load Pages from PDF"));
  }
  if (m_rotLeftBtn) {
    m_rotLeftBtn->setText(tr("Rotate Left (-90°)"));
  }
  if (m_rotRightBtn) {
    m_rotRightBtn->setText(tr("Rotate Right (+90°)"));
  }
  if (m_rot180Btn) {
    m_rot180Btn->setText(tr("Rotate 180°"));
  }
  if (m_moveUpBtn) {
    m_moveUpBtn->setText(tr("Move Up"));
  }
  if (m_moveDownBtn) {
    m_moveDownBtn->setText(tr("Move Down"));
  }
  if (m_deleteBtn) {
    m_deleteBtn->setText(tr("Delete Page"));
  }
  if (m_resetBtn) {
    m_resetBtn->setText(tr("Reset"));
  }
  if (m_pageList) {
    for (int i = 0; i < m_pageList->count(); ++i) {
      updateItemDisplay(m_pageList->item(i));
    }
  }
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
      ui::Toast::show(toastParent, tr("No PDF Selected"),
                      tr("Please add a PDF file in the drop zone first."),
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
        toastParent, tr("Cannot Load PDF Pages"),
        tr("Could not open '%1' (damaged or password-protected).")
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

bool OrganizePdfPanel::execute(const QStringList &dropPaths) {
  QWidget *toastParent = window() ? window() : this;
  if (dropPaths.isEmpty()) {
    ui::Toast::show(toastParent, tr("No PDF Selected"),
                    tr("Please select a PDF file to organize."),
                    ui::ToastVariant::Warning);
    return false;
  }

  if (m_sourcePath != dropPaths.first() || pageCount() == 0) {
    loadFromPaths(dropPaths, false);
  }
  if (pageCount() == 0) {
    return false;
  }

  const QVector<OrganizeController::PageOp> ops = pageOps();
  const QString currentSourcePath = sourcePath();
  const QFileInfo sourceInfo(currentSourcePath);
  const QString defaultOutput =
      sourceInfo.dir().filePath(sourceInfo.baseName() + "_organized.pdf");
  const QString outputPath = QFileDialog::getSaveFileName(
      this, tr("Save Organized PDF"), defaultOutput, tr("PDF Files (*.pdf)"));
  if (outputPath.isEmpty()) {
    return false;
  }

  if (pdfutil::isSamePdf(sourceInfo, QFileInfo(outputPath))) {
    ui::Toast::show(
        toastParent, tr("Invalid Output File"),
        tr("Choose a different output file. The original PDF must not "
           "be overwritten."),
        ui::ToastVariant::Destructive);
    return false;
  }

  if (!m_controller) {
    return false;
  }

  OrganizeController::Params params;
  params.sourcePath = currentSourcePath;
  params.outputPath = outputPath;
  params.pageOps = ops;

  m_controller->start(params);
  return true;
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
    ui::ScrollArea::applyScrollBarStyles(m_pageList);
  }
}

void OrganizePdfPanel::rotateSelectedPages(int deltaDegrees) {
  if (!m_pageList) {
    return;
  }
  const QList<QListWidgetItem *> selected = m_pageList->selectedItems();
  if (selected.isEmpty()) {
    ui::Toast::show(window() ? window() : this, tr("No Page Selected"),
                    tr("Select one or more pages in the list to rotate."),
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
    ui::Toast::show(toastParent, tr("No Page Selected"),
                    tr("Select one or more pages in the list to delete."),
                    ui::ToastVariant::Warning);
    return;
  }
  if (m_pageList->count() - selected.size() < 1) {
    ui::Toast::show(toastParent, tr("Cannot Delete All Pages"),
                    tr("At least one page must remain in the document."),
                    ui::ToastVariant::Warning);
    return;
  }
  for (QListWidgetItem *item : selected) {
    delete m_pageList->takeItem(m_pageList->row(item));
  }
}

void OrganizePdfPanel::onOrganizeFinished(
    const OrganizeController::Result &result) {
  emit executionFinished();

  QWidget *toastParent = window() ? window() : this;
  if (result.cancelled) {
    ui::Toast::show(toastParent, tr("Cancelled"),
                    tr("Page organization was cancelled."),
                    ui::ToastVariant::Default);
  } else if (result.success) {
    ui::Dialog::info(
        this, tr("Organized PDF Saved"),
        tr("Saved %1 page(s) to:\n%2\n\nOutput Size: %3")
            .arg(result.pageCount)
            .arg(result.outputPath)
            .arg(DropFileWidget::formatFileSize(result.outputBytes)));
  } else {
    ui::Toast::show(toastParent, tr("Failed to Save Organized PDF"),
                    result.error, ui::ToastVariant::Destructive);
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

  item->setText(tr("Page %1 (%2 × %3 pt) — Rotation: %4°")
                    .arg(origIndex + 1)
                    .arg(qRound(ptSize.width()))
                    .arg(qRound(ptSize.height()))
                    .arg(rot));
}
