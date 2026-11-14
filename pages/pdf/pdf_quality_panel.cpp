#include "pdf_quality_panel.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "widgets/drop_file_widget.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QStringList>
#include <QtGlobal>

namespace {

struct QualityPreset {
  int index;
  int quality;
};

constexpr QualityPreset kQualityPresets[] = {
    {1, 20},
    {2, 50},
    {3, 80},
};

struct PdfJobTargets {
  bool isBatch{false};
  QString sourcePath;
  QString outputPath;
  QStringList sourcePaths;
  QString outputDir;
};

bool promptPdfJobTargets(QWidget *dialogParent, QWidget *toastHost,
                         const QStringList &sourcePaths,
                         const QString &saveDialogTitle,
                         const QString &defaultSuffix,
                         const QString &batchDialogTitle,
                         PdfJobTargets &outTargets) {
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(toastHost, "No PDF Selected",
                    "Please select at least one PDF file.",
                    ui::ToastVariant::Warning);
    return false;
  }

  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    const QFileInfo sourceInfo(sourcePath);
    const QString outputPath = QFileDialog::getSaveFileName(
        dialogParent, saveDialogTitle,
        sourceInfo.dir().filePath(sourceInfo.baseName() + defaultSuffix),
        "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return false;
    }

    if (pdfutil::isSamePdf(sourceInfo, QFileInfo(outputPath))) {
      ui::Toast::show(
          toastHost, "Invalid Output File",
          "Choose a different output file. The original PDF must not "
          "be overwritten.",
          ui::ToastVariant::Destructive);
      return false;
    }

    outTargets.isBatch = false;
    outTargets.sourcePath = sourcePath;
    outTargets.outputPath = outputPath;
    return true;
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      dialogParent, batchDialogTitle, QDir::homePath());
  if (outputDir.isEmpty()) {
    return false;
  }

  outTargets.isBatch = true;
  outTargets.sourcePaths = sourcePaths;
  outTargets.outputDir = outputDir;
  return true;
}

template <typename Controller, typename Params>
bool launchQualityJob(QWidget *dialogParent, QWidget *toastHost,
                      Controller *controller, const QStringList &sourcePaths,
                      int quality, const QString &saveDialogTitle,
                      const QString &defaultSuffix,
                      const QString &batchDialogTitle, int &outBatchCount) {
  if (!controller) {
    return false;
  }

  PdfJobTargets targets;
  if (!promptPdfJobTargets(dialogParent, toastHost, sourcePaths,
                           saveDialogTitle, defaultSuffix, batchDialogTitle,
                           targets)) {
    return false;
  }

  Params params;
  params.isBatch = targets.isBatch;
  params.sourcePath = targets.sourcePath;
  params.outputPath = targets.outputPath;
  params.sourcePaths = targets.sourcePaths;
  params.outputDir = targets.outputDir;
  params.quality = quality;

  outBatchCount = static_cast<int>(sourcePaths.size());
  controller->start(params);
  return true;
}

void appendBatchFooter(QString &message, const QStringList &errors,
                       bool cancelled, int notCompleted) {
  if (!errors.isEmpty()) {
    message += "\n\n" + errors.join("\n");
  }
  if (cancelled) {
    message += QString("\n%1 PDF(s) not completed due to cancellation.")
                   .arg(qMax(0, notCompleted));
  }
}

} // namespace

PdfQualityPanel::PdfQualityPanel(QWidget *parent) : QWidget(parent) {
  auto *attributeLayout = new QHBoxLayout(this);
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setValue(50);

  const QStringList presetOptions = {"Custom", "Web / Email", "Balanced",
                                     "High Quality (preserve images)"};
  m_presetSelect = new ui::Select(presetOptions, this);
  m_presetSelect->setCurrentIndex(2); // Default to Balanced (50)
  m_presetField =
      new ui::FormField("Compression Preset", m_presetSelect, this);
  m_presetField->setToolTip(
      "Text and vector graphics are retained. Below quality 80, images may "
      "lose detail. At quality 80 or above, images are not downsampled or "
      "newly JPEG-encoded. PDF rewriting can alter forms and annotations "
      "and invalidates digital signatures. Keep the original PDF.");

  connect(m_presetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfQualityPanel::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfQualityPanel::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_presetField, 1, Qt::AlignTop);

  m_compressController = new CompressController(this);
  m_grayscaleController = new GrayscaleController(this);
  connect(m_compressController, &CompressController::finished, this,
          &PdfQualityPanel::onCompressFinished, Qt::QueuedConnection);
  connect(m_grayscaleController, &GrayscaleController::finished, this,
          &PdfQualityPanel::onGrayscaleFinished, Qt::QueuedConnection);
}

int PdfQualityPanel::quality() const {
  return m_qualitySlider ? m_qualitySlider->getValue() : 50;
}

void PdfQualityPanel::setPresetVisible(bool visible) {
  if (m_presetField) {
    m_presetField->setVisible(visible);
  }
}

void PdfQualityPanel::setControlsEnabled(bool enabled) {
  if (m_qualitySlider) {
    m_qualitySlider->setEnabled(enabled);
  }
  if (m_presetSelect) {
    m_presetSelect->setEnabled(enabled);
  }
}

void PdfQualityPanel::onPresetChanged(int index) {
  if (!m_qualitySlider) {
    return;
  }
  for (const auto &preset : kQualityPresets) {
    if (preset.index == index) {
      m_qualitySlider->blockSignals(true);
      m_qualitySlider->setValue(preset.quality);
      m_qualitySlider->blockSignals(false);
      return;
    }
  }
}

void PdfQualityPanel::onQualitySliderChanged() {
  if (!m_presetSelect || !m_qualitySlider) {
    return;
  }
  const int val = m_qualitySlider->getValue();
  int presetIndex = 0;
  for (const auto &preset : kQualityPresets) {
    if (preset.quality == val) {
      presetIndex = preset.index;
      break;
    }
  }
  m_presetSelect->blockSignals(true);
  m_presetSelect->setCurrentIndex(presetIndex);
  m_presetSelect->blockSignals(false);
}

bool PdfQualityPanel::executeCompress(const QStringList &sourcePaths) {
  QWidget *toastHost = window() ? window() : this;
  return launchQualityJob<CompressController, CompressController::Params>(
      this, toastHost, m_compressController, sourcePaths, quality(),
      "Save Compressed PDF", "_compressed.pdf",
      "Select Output Directory for Compressed PDFs", m_lastBatchCount);
}

bool PdfQualityPanel::executeGrayscale(const QStringList &sourcePaths) {
  QWidget *toastHost = window() ? window() : this;
  return launchQualityJob<GrayscaleController, GrayscaleController::Params>(
      this, toastHost, m_grayscaleController, sourcePaths, quality(),
      "Save Grayscale PDF", "_grayscale.pdf",
      "Select Output Directory for Grayscale PDFs", m_lastBatchCount);
}

void PdfQualityPanel::onCompressFinished(
    const CompressController::Result &result) {
  emit executionFinished();

  QWidget *toastHost = window() ? window() : this;
  if (!result.isBatch) {
    if (result.status == CompressController::Result::Status::Compressed) {
      const double savedPercent =
          result.origBytes > 0
              ? 100.0 *
                    (1.0 - static_cast<double>(result.outBytes) /
                               result.origBytes)
              : 0.0;
      ui::Dialog::info(
          this, "PDF Compressed",
          QString("Saved to: %1\n\nOriginal: %2\nCompressed: %3 "
                  "(%4% smaller)")
              .arg(result.singleOutputPath)
              .arg(DropFileWidget::formatFileSize(result.origBytes))
              .arg(DropFileWidget::formatFileSize(result.outBytes))
              .arg(savedPercent, 0, 'f', 1));
    } else if (result.status ==
               CompressController::Result::Status::Unchanged) {
      ui::Dialog::info(
          this, "No Reduction Achieved",
          "These settings did not produce a smaller PDF. "
          "The original PDF and destination were left unchanged.");
    } else if (result.status ==
               CompressController::Result::Status::Cancelled) {
      ui::Toast::show(toastHost, "Cancelled",
                      "PDF compression was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(toastHost, "PDF Compression Failed",
                      result.errors.isEmpty() ? "Compression failed."
                                              : result.errors.first(),
                      ui::ToastVariant::Destructive);
    }
    return;
  }

  const bool isCancelled =
      result.status == CompressController::Result::Status::Cancelled;
  const QString title = isCancelled ? "Batch Compression Cancelled"
                                    : "Batch Compression Complete";
  QString message =
      QString("%1 PDF(s) compressed.\n%2 PDF(s) unchanged (no output "
              "saved).\n%3 PDF(s) failed.")
          .arg(result.successCount)
          .arg(result.unchangedCount)
          .arg(result.failCount);

  if (result.origBytes > 0 && result.outBytes > 0 && result.successCount > 0) {
    const double diffPct =
        (1.0 - (static_cast<double>(result.outBytes) / result.origBytes)) *
        100.0;
    message +=
        QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
            .arg(DropFileWidget::formatFileSize(result.origBytes))
            .arg(DropFileWidget::formatFileSize(result.outBytes))
            .arg(qAbs(diffPct), 0, 'f', 1)
            .arg(diffPct >= 0 ? "smaller" : "larger");
  }

  appendBatchFooter(message, result.errors, isCancelled,
                    m_lastBatchCount - result.successCount -
                        result.unchangedCount - result.failCount);
  ui::Dialog::info(this, title, message);
}

void PdfQualityPanel::onGrayscaleFinished(
    const GrayscaleController::Result &result) {
  emit executionFinished();

  QWidget *toastHost = window() ? window() : this;
  if (!result.isBatch) {
    if (result.success) {
      ui::Dialog::info(
          this, "Grayscale PDF Saved",
          QString("Saved to: %1\n\nOriginal Size: %2\nGrayscale Size: %3")
              .arg(result.singleOutputPath)
              .arg(DropFileWidget::formatFileSize(result.origBytes))
              .arg(DropFileWidget::formatFileSize(result.outBytes)));
    } else if (result.cancelled) {
      ui::Toast::show(toastHost, "Cancelled",
                      "Grayscale conversion was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(toastHost, "Grayscale Conversion Failed", result.error,
                      ui::ToastVariant::Destructive);
    }
    return;
  }

  const QString title = result.cancelled
                            ? "Batch Grayscale Conversion Cancelled"
                            : "Batch Grayscale Conversion Complete";
  QString message = QString("%1 PDF(s) converted to grayscale.\n"
                            "%2 PDF(s) failed.")
                        .arg(result.successCount)
                        .arg(result.failCount);

  if (result.origBytes > 0 && result.outBytes > 0 && result.successCount > 0) {
    message += QString("\n\nTotal Original: %1\nTotal Grayscale: %2")
                   .arg(DropFileWidget::formatFileSize(result.origBytes))
                   .arg(DropFileWidget::formatFileSize(result.outBytes));
  }

  appendBatchFooter(message, result.errors, result.cancelled,
                    m_lastBatchCount - result.successCount - result.failCount);
  ui::Dialog::info(this, title, message);
}
