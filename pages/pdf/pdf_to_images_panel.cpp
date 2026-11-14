#include "pdf_to_images_panel.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPair>
#include <QPdfDocument>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

PdfToImagesPanel::PdfToImagesPanel(QWidget *parent) : QWidget(parent) {
  auto *exportLayout = new QHBoxLayout(this);
  exportLayout->setContentsMargins(0, 0, 0, 0);
  exportLayout->setSpacing(12);

  const QStringList exportFormats = {"PNG", "JPG", "WEBP", "TIFF"};
  m_formatSelect = new ui::Select(exportFormats, this);
  m_formatField = new ui::FormField("Image Format", m_formatSelect, this);
  connect(m_formatSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfToImagesPanel::exportFormatChanged);

  const QStringList dpiOptions = {"72 DPI (Screen)", "150 DPI (Medium)",
                                  "300 DPI (Print)", "600 DPI (High Detail)"};
  m_dpiSelect = new ui::Select(dpiOptions, this);
  m_dpiSelect->setCurrentIndex(2); // Default 300 DPI
  m_dpiField = new ui::FormField("Render Resolution", m_dpiSelect, this);

  m_pageRangeInput = new ui::Input("All pages, or e.g. 1-3, 5", this);
  m_pageRangeField =
      new ui::FormField("Page Filter (optional)", m_pageRangeInput, this);

  exportLayout->addWidget(m_formatField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_dpiField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_pageRangeField, 2, Qt::AlignTop);

  m_controller = new PdfToImagesController(this);
  connect(m_controller, &PdfToImagesController::finished, this,
          &PdfToImagesPanel::onPdfToImagesFinished, Qt::QueuedConnection);
}

QString PdfToImagesPanel::selectedFormat() const {
  const QStringList formats = {"png", "jpg", "webp", "tiff"};
  return formats.value(formatIndex(), "png");
}

int PdfToImagesPanel::formatIndex() const {
  return m_formatSelect ? m_formatSelect->currentIndex() : 0;
}

int PdfToImagesPanel::selectedDpi() const {
  const int dpiIndex = m_dpiSelect ? m_dpiSelect->currentIndex() : 2;
  const QVector<int> dpis = {72, 150, 300, 600};
  return dpis.value(dpiIndex, 300);
}

QString PdfToImagesPanel::pageRangeSpec() const {
  return m_pageRangeInput ? m_pageRangeInput->text().trimmed() : QString();
}

bool PdfToImagesPanel::execute(const QStringList &sourcePaths, int quality) {
  QWidget *toastParent = window() ? window() : this;

  if (sourcePaths.isEmpty()) {
    ui::Toast::show(toastParent, "No PDF Selected",
                    "Please select at least one PDF file to export.",
                    ui::ToastVariant::Warning);
    return false;
  }

  const QString format = selectedFormat();
  const int effectiveQuality = (format == "png") ? -1 : quality;
  const int dpi = selectedDpi();
  const QString rangeSpec = pageRangeSpec();

  if (!rangeSpec.isEmpty()) {
    QPdfDocument checkDoc;
    if (checkDoc.load(sourcePaths.first()) != QPdfDocument::Error::None ||
        checkDoc.pageCount() <= 0) {
      ui::Toast::show(toastParent, "Invalid PDF",
                      QString("Cannot open '%1'.")
                          .arg(QFileInfo(sourcePaths.first()).fileName()),
                      ui::ToastVariant::Destructive);
      return false;
    }
    QVector<QPair<int, int>> testRanges;
    QString rangeErr;
    if (!pdfutil::parsePageRanges(rangeSpec, checkDoc.pageCount(), testRanges,
                                  rangeErr)) {
      ui::Toast::show(toastParent, "Invalid Page Filter", rangeErr,
                      ui::ToastVariant::Warning);
      return false;
    }
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Exported Page Images",
      QDir::homePath());
  if (outputDir.isEmpty()) {
    return false;
  }

  PdfToImagesController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.format = format;
  params.dpi = dpi;
  params.rangeFilter = rangeSpec;
  params.quality = effectiveQuality;

  m_controller->start(params);
  return true;
}

void PdfToImagesPanel::onPdfToImagesFinished(
    const PdfToImagesController::Result &result) {
  emit executionFinished();

  QWidget *toastParent = window() ? window() : this;
  if (result.cancelled) {
    ui::Toast::show(toastParent, "Cancelled", "Page image export was cancelled.",
                    ui::ToastVariant::Default);
  } else if (result.imagesExported > 0 && result.failedDocs == 0) {
    ui::Dialog::info(
        this, "PDF Pages Exported",
        QString("Exported %1 %2 image(s) at %3 DPI to:\n%4")
            .arg(result.imagesExported)
            .arg(result.format.toUpper())
            .arg(result.dpi)
            .arg(result.outputDir));
  } else if (result.imagesExported > 0) {
    ui::Dialog::info(this, "Export Completed with Warnings",
                     QString("Exported %1 %2 image(s) to:\n%3\n\nErrors:\n%4")
                         .arg(result.imagesExported)
                         .arg(result.format.toUpper())
                         .arg(result.outputDir)
                         .arg(result.errors.join("\n")));
  } else {
    ui::Toast::show(toastParent, "Export Failed",
                    result.errors.isEmpty()
                        ? "Could not export pages to images."
                        : result.errors.join("\n"),
                    ui::ToastVariant::Destructive);
  }
}
