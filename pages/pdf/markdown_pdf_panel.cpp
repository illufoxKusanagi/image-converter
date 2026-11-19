#include "markdown_pdf_panel.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/i18n.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QString>
#include <QStringList>
#include <QVector>

namespace {

ui::FormField *createSelectField(QWidget *parent, QHBoxLayout *layout,
                                 const QString &label,
                                 const QStringList &items, int defaultIndex,
                                 ui::Select *&outSelect) {
  outSelect = new ui::Select(items, parent);
  if (defaultIndex > 0) {
    outSelect->setCurrentIndex(defaultIndex);
  }
  auto *field = new ui::FormField(label, outSelect, parent);
  layout->addWidget(field, 1, Qt::AlignTop);
  return field;
}

} // namespace

MarkdownPdfPanel::MarkdownPdfPanel(QWidget *parent) : QWidget(parent) {
  m_controller = new MarkdownPdfController(this);
  connect(m_controller, &MarkdownPdfController::finished, this,
          &MarkdownPdfPanel::onMarkdownPdfFinished, Qt::QueuedConnection);

  auto *mdLayout = new QHBoxLayout(this);
  mdLayout->setContentsMargins(0, 0, 0, 0);
  mdLayout->setSpacing(12);

  const QStringList pageSizes = {tr("A4 (210 × 297 mm)"),
                                 tr("US Letter (8.5 × 11 in)")};
  m_pageSizeField = createSelectField(this, mdLayout, tr("Page Size"),
                                      pageSizes, 0, m_pageSizeSelect);

  const QStringList orientations = {tr("Portrait"), tr("Landscape")};
  m_orientationField = createSelectField(this, mdLayout, tr("Orientation"),
                                         orientations, 0, m_orientationSelect);

  const QStringList margins = {tr("Compact (18 pt)"), tr("Standard (36 pt)"),
                               tr("Wide (54 pt)"), tr("Extra Wide (72 pt)")};
  m_marginField = createSelectField(this, mdLayout, tr("Page Margins"), margins,
                                    1, m_marginSelect);

  const QStringList fontSizes = {tr("9 pt"), tr("10 pt (Default)"), tr("11 pt"),
                                 tr("12 pt"), tr("14 pt"), tr("16 pt")};
  m_fontSizeField = createSelectField(this, mdLayout, tr("Base Font Size"),
                                      fontSizes, 1, m_fontSizeSelect);

  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &MarkdownPdfPanel::retranslateUi);
}

void MarkdownPdfPanel::retranslateUi() {
  auto retranslateSelect = [](ui::FormField *field, ui::Select *select,
                              const QString &label, const QStringList &items) {
    if (field) {
      field->setLabel(label);
    }
    if (select) {
      QSignalBlocker blocker(select);
      for (int i = 0; i < items.size() && i < select->count(); ++i) {
        select->setItemText(i, items.at(i));
      }
    }
  };

  retranslateSelect(
      m_pageSizeField, m_pageSizeSelect, tr("Page Size"),
      {tr("A4 (210 × 297 mm)"), tr("US Letter (8.5 × 11 in)")});
  retranslateSelect(m_orientationField, m_orientationSelect, tr("Orientation"),
                    {tr("Portrait"), tr("Landscape")});
  retranslateSelect(m_marginField, m_marginSelect, tr("Page Margins"),
                    {tr("Compact (18 pt)"), tr("Standard (36 pt)"),
                     tr("Wide (54 pt)"), tr("Extra Wide (72 pt)")});
  retranslateSelect(m_fontSizeField, m_fontSizeSelect, tr("Base Font Size"),
                    {tr("9 pt"), tr("10 pt (Default)"), tr("11 pt"),
                     tr("12 pt"), tr("14 pt"), tr("16 pt")});
}

bool MarkdownPdfPanel::useLetterPageSize() const {
  return m_pageSizeSelect && m_pageSizeSelect->currentIndex() == 1;
}

bool MarkdownPdfPanel::landscape() const {
  return m_orientationSelect && m_orientationSelect->currentIndex() == 1;
}

int MarkdownPdfPanel::marginPt() const {
  const QVector<int> marginOptions = {18, 36, 54, 72};
  return marginOptions.value(
      m_marginSelect ? m_marginSelect->currentIndex() : 1, 36);
}

int MarkdownPdfPanel::baseFontPt() const {
  const QVector<int> fontOptions = {9, 10, 11, 12, 14, 16};
  return fontOptions.value(
      m_fontSizeSelect ? m_fontSizeSelect->currentIndex() : 1, 10);
}

bool MarkdownPdfPanel::execute(const QStringList &sourcePaths) {
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(
        window() ? window() : this, tr("No Document Selected"),
        tr("Please select at least one Markdown (.md) or HTML (.html) file."),
        ui::ToastVariant::Warning);
    return false;
  }

  const QString defaultDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, tr("Select Output Directory for Generated PDFs"), defaultDir);
  if (outputDir.isEmpty()) {
    return false;
  }

  MarkdownPdfController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.useLetterPageSize = useLetterPageSize();
  params.landscape = landscape();
  params.marginPt = marginPt();
  params.baseFontPt = baseFontPt();

  m_controller->start(params);
  return true;
}

void MarkdownPdfPanel::onMarkdownPdfFinished(
    const MarkdownPdfController::Result &result) {
  emit executionFinished();

  QWidget *toastParent = window() ? window() : this;

  if (result.cancelled) {
    ui::Toast::show(toastParent, tr("Cancelled"),
                    tr("Document PDF export was cancelled."),
                    ui::ToastVariant::Default);
    return;
  }

  if (result.pdfsCreated > 0) {
    QString msg = tr("Exported %1 document(s) to PDF in:\n%2")
                      .arg(result.pdfsCreated)
                      .arg(result.outputDir);
    if (result.failedFiles > 0) {
      msg += tr("\n\n%1 file(s) failed:\n%2")
                 .arg(result.failedFiles)
                 .arg(result.error);
    }
    ui::Dialog::info(this, tr("PDF Export Complete"), msg);
  } else {
    ui::Toast::show(toastParent, tr("PDF Export Failed"),
                    result.error.isEmpty()
                        ? tr("Failed to export Markdown/HTML to PDF.")
                        : result.error,
                    ui::ToastVariant::Destructive);
  }
}
