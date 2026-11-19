#include "merge_pdf_panel.h"
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
#include <QListWidgetItem>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

MergePdfPanel::MergePdfPanel(QWidget *parent) : QWidget(parent) {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(6);

  auto *headerLayout = new QHBoxLayout();
  headerLayout->setContentsMargins(0, 0, 0, 0);
  headerLayout->setSpacing(8);

  m_hintLabel =
      new QLabel(tr("Merge Order (drag items or use buttons to reorder):"), this);
  m_syncBtn =
      new ui::Button(tr("Sync from Drop Zone"), ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Small, this);
  m_upBtn = new ui::Button(tr("Move Up"), ui::ButtonVariant::Outline,
                           ui::ButtonSize::Small, this);
  m_downBtn = new ui::Button(tr("Move Down"), ui::ButtonVariant::Outline,
                             ui::ButtonSize::Small, this);

  connect(m_syncBtn, &QPushButton::clicked, this,
          &MergePdfPanel::syncRequested);
  connect(m_upBtn, &QPushButton::clicked, this, &MergePdfPanel::moveItemUp);
  connect(m_downBtn, &QPushButton::clicked, this, &MergePdfPanel::moveItemDown);

  headerLayout->addWidget(m_hintLabel, 1);
  headerLayout->addWidget(m_syncBtn);
  headerLayout->addWidget(m_upBtn);
  headerLayout->addWidget(m_downBtn);

  m_orderList = new QListWidget(this);
  m_orderList->setDragDropMode(QAbstractItemView::InternalMove);
  m_orderList->setDefaultDropAction(Qt::MoveAction);
  m_orderList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_orderList->setFixedHeight(115);

  mainLayout->addLayout(headerLayout);
  mainLayout->addWidget(m_orderList);

  applyThemeStyles();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MergePdfPanel::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &MergePdfPanel::retranslateUi);

  m_controller = new MergeController(this);
  connect(m_controller, &MergeController::finished, this,
          &MergePdfPanel::onMergeFinished, Qt::QueuedConnection);
}

void MergePdfPanel::syncFromPaths(const QStringList &dropPaths) {
  if (!m_orderList) {
    return;
  }

  QSet<QString> dropSet;
  dropSet.reserve(dropPaths.size());
  for (const QString &path : dropPaths) {
    dropSet.insert(path);
  }

  QStringList ordered;
  ordered.reserve(dropSet.size());
  QSet<QString> seen;
  seen.reserve(dropSet.size());

  for (int i = 0; i < m_orderList->count(); ++i) {
    const QListWidgetItem *item = m_orderList->item(i);
    if (!item) {
      continue;
    }
    const QString path = item->data(Qt::UserRole).toString();
    if (dropSet.contains(path) && !seen.contains(path)) {
      ordered.append(path);
      seen.insert(path);
    }
  }

  for (const QString &path : dropPaths) {
    if (!seen.contains(path)) {
      ordered.append(path);
      seen.insert(path);
    }
  }

  m_orderList->clear();
  for (const QString &path : ordered) {
    const QFileInfo fi(path);
    auto *item = new QListWidgetItem(
        QString("%1 (%2)").arg(fi.fileName(),
                               DropFileWidget::formatFileSize(fi.size())),
        m_orderList);
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
  }
}

QStringList MergePdfPanel::orderedPaths() const {
  QStringList paths;
  if (!m_orderList) {
    return paths;
  }
  paths.reserve(m_orderList->count());
  for (int i = 0; i < m_orderList->count(); ++i) {
    const QListWidgetItem *item = m_orderList->item(i);
    if (item) {
      paths.append(item->data(Qt::UserRole).toString());
    }
  }
  return paths;
}

int MergePdfPanel::count() const {
  return m_orderList ? m_orderList->count() : 0;
}

void MergePdfPanel::retranslateUi() {
  if (m_hintLabel) {
    m_hintLabel->setText(
        tr("Merge Order (drag items or use buttons to reorder):"));
  }
  if (m_syncBtn) {
    m_syncBtn->setText(tr("Sync from Drop Zone"));
  }
  if (m_upBtn) {
    m_upBtn->setText(tr("Move Up"));
  }
  if (m_downBtn) {
    m_downBtn->setText(tr("Move Down"));
  }
  if (m_orderList) {
    for (int i = 0; i < m_orderList->count(); ++i) {
      if (QListWidgetItem *item = m_orderList->item(i)) {
        const QFileInfo fi(item->data(Qt::UserRole).toString());
        item->setText(QString("%1 (%2)").arg(
            fi.fileName(), DropFileWidget::formatFileSize(fi.size())));
      }
    }
  }
}

void MergePdfPanel::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  if (m_hintLabel) {
    m_hintLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_hintLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.foreground)));
  }

  if (m_orderList) {
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

    m_orderList->setFont(t.font(t.sizeSm));
    m_orderList->setStyleSheet(listStyle);
    ui::ScrollArea::applyScrollBarStyles(m_orderList);
  }
}

void MergePdfPanel::moveItemUp() { moveCurrentItem(-1); }

void MergePdfPanel::moveItemDown() { moveCurrentItem(1); }

void MergePdfPanel::moveCurrentItem(int delta) {
  if (!m_orderList || delta == 0) {
    return;
  }
  const int row = m_orderList->currentRow();
  const int targetRow = row + delta;
  if (row < 0 || targetRow < 0 || targetRow >= m_orderList->count()) {
    return;
  }
  QListWidgetItem *item = m_orderList->takeItem(row);
  if (!item) {
    return;
  }
  m_orderList->insertItem(targetRow, item);
  m_orderList->setCurrentRow(targetRow);
}

bool MergePdfPanel::execute(const QStringList &dropPaths) {
  syncFromPaths(dropPaths);

  const QStringList paths = orderedPaths();
  QWidget *const toastParent = window() ? window() : this;

  if (paths.size() < 2) {
    ui::Toast::show(toastParent, tr("Not Enough PDFs"),
                    tr("Please select at least 2 PDF files to merge."),
                    ui::ToastVariant::Warning);
    return false;
  }

  const QFileInfo firstInfo(paths.first());
  const QString defaultOutput =
      firstInfo.dir().filePath(firstInfo.baseName() + "_merged.pdf");
  const QString outputPath = QFileDialog::getSaveFileName(
      this, tr("Save Merged PDF"), defaultOutput, tr("PDF Files (*.pdf)"));
  if (outputPath.isEmpty()) {
    return false;
  }

  const QFileInfo destInfo(outputPath);
  for (const QString &srcPath : paths) {
    if (pdfutil::isSamePdf(QFileInfo(srcPath), destInfo)) {
      ui::Toast::show(
          toastParent, tr("Invalid Output File"),
          tr("The merged PDF destination cannot overwrite one of the input "
             "PDFs."),
          ui::ToastVariant::Destructive);
      return false;
    }
  }

  if (!m_controller) {
    return false;
  }

  MergeController::Params params;
  params.inputPaths = paths;
  params.outputPath = outputPath;

  m_controller->start(params);
  return true;
}

void MergePdfPanel::onMergeFinished(const MergeController::Result &result) {
  emit executionFinished();

  QWidget *const toastParent = window() ? window() : this;
  if (result.cancelled) {
    ui::Toast::show(toastParent, tr("Cancelled"),
                    tr("PDF merge was cancelled."), ui::ToastVariant::Default);
  } else if (result.success) {
    ui::Dialog::info(
        this, tr("PDFs Merged"),
        tr("Merged %1 PDFs (%2 total pages) into:\n%3\n\n"
           "Output Size: %4")
            .arg(count())
            .arg(result.totalPages)
            .arg(result.outputPath)
            .arg(DropFileWidget::formatFileSize(result.outputBytes)));
  } else {
    ui::Toast::show(toastParent, tr("PDF Merge Failed"), result.error,
                    ui::ToastVariant::Destructive);
  }
}
