#include "markdown_pdf_panel.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
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

  const QStringList pageSizes = {"A4 (210 × 297 mm)",
                                 "US Letter (8.5 × 11 in)"};
  m_pageSizeField = createSelectField(this, mdLayout, "Page Size", pageSizes,
                                      0, m_pageSizeSelect);

  const QStringList orientations = {"Portrait", "Landscape"};
  m_orientationField = createSelectField(this, mdLayout, "Orientation",
                                         orientations, 0, m_orientationSelect);

  const QStringList margins = {"Compact (18 pt)", "Standard (36 pt)",
                               "Wide (54 pt)", "Extra Wide (72 pt)"};
  m_marginField = createSelectField(this, mdLayout, "Page Margins", margins, 1,
                                    m_marginSelect);

  const QStringList fontSizes = {"9 pt", "10 pt (Default)", "11 pt",
                                 "12 pt", "14 pt", "16 pt"};
  m_fontSizeField = createSelectField(this, mdLayout, "Base Font Size",
                                      fontSizes, 1, m_fontSizeSelect);
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
        window() ? window() : this, "No Document Selected",
        "Please select at least one Markdown (.md) or HTML (.html) file.",
        ui::ToastVariant::Warning);
    return false;
  }

  const QString defaultDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Generated PDFs", defaultDir);
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
    ui::Toast::show(toastParent, "Cancelled",
                    "Document PDF export was cancelled.",
                    ui::ToastVariant::Default);
    return;
  }

  if (result.pdfsCreated > 0) {
    QString msg = QString("Exported %1 document(s) to PDF in:\n%2")
                      .arg(result.pdfsCreated)
                      .arg(result.outputDir);
    if (result.failedFiles > 0) {
      msg += QString("\n\n%1 file(s) failed:\n%2")
                 .arg(result.failedFiles)
                 .arg(result.error);
    }
    ui::Dialog::info(this, "PDF Export Complete", msg);
  } else {
    ui::Toast::show(toastParent, "PDF Export Failed",
                    result.error.isEmpty()
                        ? "Failed to export Markdown/HTML to PDF."
                        : result.error,
                    ui::ToastVariant::Destructive);
  }
}
