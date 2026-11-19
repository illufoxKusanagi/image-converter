#include "pdf_to_images_panel.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/i18n.h"
#include "widgets/drop_file_widget.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPair>
#include <QPdfDocument>
#include <QSignalBlocker>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

PdfToImagesPanel::PdfToImagesPanel(QWidget *parent) : QWidget(parent) {
  auto *exportLayout = new QHBoxLayout(this);
  exportLayout->setContentsMargins(0, 0, 0, 0);
  exportLayout->setSpacing(12);

  const QStringList exportFormats = {"PNG", "JPG", "WEBP", "TIFF", "BMP", "GIF"};
  m_formatSelect = new ui::Select(exportFormats, this);
  m_formatField = new ui::FormField(tr("Image Format"), m_formatSelect, this);
  connect(m_formatSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfToImagesPanel::exportFormatChanged);

  const QStringList dpiOptions = {
      tr("72 DPI (Screen)"), tr("150 DPI (Medium)"), tr("300 DPI (Print)"),
      tr("600 DPI (High Detail)")};
  m_dpiSelect = new ui::Select(dpiOptions, this);
  m_dpiSelect->setCurrentIndex(2); // Default 300 DPI
  m_dpiField = new ui::FormField(tr("Render Resolution"), m_dpiSelect, this);

  m_pageRangeInput = new ui::Input(tr("All pages, or e.g. 1-3, 5"), this);
  m_pageRangeField =
      new ui::FormField(tr("Page Filter (optional)"), m_pageRangeInput, this);

  exportLayout->addWidget(m_formatField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_dpiField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_pageRangeField, 2, Qt::AlignTop);

  m_controller = new PdfToImagesController(this);
  connect(m_controller, &PdfToImagesController::finished, this,
          &PdfToImagesPanel::onPdfToImagesFinished, Qt::QueuedConnection);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &PdfToImagesPanel::retranslateUi);
}

void PdfToImagesPanel::retranslateUi() {
  if (m_formatField) {
    m_formatField->setLabel(tr("Image Format"));
  }
  if (m_dpiField) {
    m_dpiField->setLabel(tr("Render Resolution"));
  }
  if (m_dpiSelect) {
    QSignalBlocker blocker(m_dpiSelect);
    const QStringList dpiOptions = {
        tr("72 DPI (Screen)"), tr("150 DPI (Medium)"), tr("300 DPI (Print)"),
        tr("600 DPI (High Detail)")};
    for (int i = 0; i < dpiOptions.size() && i < m_dpiSelect->count(); ++i) {
      m_dpiSelect->setItemText(i, dpiOptions.at(i));
    }
  }
  if (m_pageRangeField) {
    m_pageRangeField->setLabel(tr("Page Filter (optional)"));
  }
  if (m_pageRangeInput) {
    m_pageRangeInput->setPlaceholderText(tr("All pages, or e.g. 1-3, 5"));
  }
}

QString PdfToImagesPanel::selectedFormat() const {
  const QStringList formats = {"png", "jpg", "webp", "tiff", "bmp", "gif"};
  return formats.value(formatIndex(), "png");
}

bool PdfToImagesPanel::supportsQuality() const {
  const QString format = selectedFormat();
  return format == "jpg" || format == "webp" || format == "tiff";
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
    ui::Toast::show(toastParent, tr("No PDF Selected"),
                    tr("Please select at least one PDF file to export."),
                    ui::ToastVariant::Warning);
    return false;
  }

  const QString format = selectedFormat();
  const int effectiveQuality = supportsQuality() ? quality : -1;
  const int dpi = selectedDpi();
  const QString rangeSpec = pageRangeSpec();

  if (!rangeSpec.isEmpty()) {
    QPdfDocument checkDoc;
    if (checkDoc.load(sourcePaths.first()) != QPdfDocument::Error::None ||
        checkDoc.pageCount() <= 0) {
      ui::Toast::show(toastParent, tr("Invalid PDF"),
                      tr("Cannot open '%1'.")
                          .arg(QFileInfo(sourcePaths.first()).fileName()),
                      ui::ToastVariant::Destructive);
      return false;
    }
    QVector<QPair<int, int>> testRanges;
    QString rangeErr;
    if (!pdfutil::parsePageRanges(rangeSpec, checkDoc.pageCount(), testRanges,
                                  rangeErr)) {
      ui::Toast::show(toastParent, tr("Invalid Page Filter"), rangeErr,
                      ui::ToastVariant::Warning);
      return false;
    }
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      this, tr("Select Output Directory for Exported Page Images"),
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
    ui::Toast::show(toastParent, tr("Cancelled"),
                    tr("Page image export was cancelled."),
                    ui::ToastVariant::Default);
  } else if (result.imagesExported > 0 && result.failedDocs == 0) {
    ui::Dialog::info(
        this, tr("PDF Pages Exported"),
        tr("Exported %1 %2 image(s) at %3 DPI to:\n%4\n\nTotal Size: %5")
            .arg(result.imagesExported)
            .arg(result.format.toUpper())
            .arg(result.dpi)
            .arg(result.outputDir)
            .arg(DropFileWidget::formatFileSize(result.totalOutputBytes)));
  } else if (result.imagesExported > 0) {
    ui::Dialog::info(
        this, tr("Export Completed with Warnings"),
        tr("Exported %1 %2 image(s) at %3 DPI to:\n%4\n\nTotal Size: %5\n\nErrors:\n%6")
            .arg(result.imagesExported)
            .arg(result.format.toUpper())
            .arg(result.dpi)
            .arg(result.outputDir)
            .arg(DropFileWidget::formatFileSize(result.totalOutputBytes))
            .arg(result.errors.join("\n")));
  } else {
    ui::Toast::show(toastParent, tr("Export Failed"),
                    result.errors.isEmpty()
                        ? tr("Could not export pages to images.")
                        : result.errors.join("\n"),
                    ui::ToastVariant::Destructive);
  }
}
