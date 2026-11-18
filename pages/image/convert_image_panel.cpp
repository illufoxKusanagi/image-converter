#include "pages/image/convert_image_panel.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QtGlobal>

namespace {

QString formatSizeDiffStats(qint64 origBytes, qint64 outBytes,
                            const QString &origLabel,
                            const QString &outLabel) {
  if (origBytes <= 0 || outBytes <= 0) {
    return QString();
  }
  const double diffPct =
      (1.0 - (static_cast<double>(outBytes) / origBytes)) * 100.0;
  return QString("\n\n%1: %2\n%3: %4 (%5% %6)")
      .arg(origLabel)
      .arg(DropFileWidget::formatFileSize(origBytes))
      .arg(outLabel)
      .arg(DropFileWidget::formatFileSize(outBytes))
      .arg(qAbs(diffPct), 0, 'f', 1)
      .arg(diffPct >= 0 ? "smaller" : "larger");
}

} // namespace

ConvertImagePanel::ConvertImagePanel(QWidget *parent)
    : QWidget(parent), m_controller(new ImageConvertController(this)) {
  auto *attributeLayout = new QHBoxLayout(this);
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setMinimumWidth(250);

  const QStringList extensionOptions = {"JPG",  "JPEG", "PNG", "WEBP",
                                        "TIFF", "BMP",  "GIF", "PDF"};
  m_targetExtensionSelect = new ui::Select(extensionOptions, this);
  m_targetExtensionSelect->setFixedHeight(36);
  m_targetExtensionField =
      new ui::FormField("Target Format", m_targetExtensionSelect, this);
  m_targetExtensionField->setMinimumWidth(160);

  connect(m_targetExtensionSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ConvertImagePanel::onExtensionChanged);
  connect(m_controller, &ImageConvertController::finished, this,
          &ConvertImagePanel::onConvertFinished, Qt::QueuedConnection);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_targetExtensionField, 1, Qt::AlignTop);

  onExtensionChanged(m_targetExtensionSelect->currentIndex());
}

QImage ConvertImagePanel::loadRasterOrSvgImage(const QString &path,
                                               bool forceOpaqueWhite) {
  return ImageConvertController::loadRasterOrSvgImage(path, forceOpaqueWhite);
}

void ConvertImagePanel::syncProcessingState(bool isProcessing) {
  if (isProcessing) {
    if (m_targetExtensionSelect) {
      m_targetExtensionSelect->setEnabled(false);
    }
    if (m_qualitySlider) {
      m_qualitySlider->setEnabled(false);
    }
  } else {
    if (m_targetExtensionSelect) {
      m_targetExtensionSelect->setEnabled(true);
      onExtensionChanged(m_targetExtensionSelect->currentIndex());
    } else if (m_qualitySlider) {
      m_qualitySlider->setEnabled(true);
    }
  }
}

void ConvertImagePanel::onExtensionChanged(int index) {
  if (!m_qualitySlider) {
    return;
  }

  if (index == 0) {
    m_targetExtension = DropFileWidget::ImageExtension::JPG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (index == 1) {
    m_targetExtension = DropFileWidget::ImageExtension::JPEG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (index == 2) {
    m_targetExtension = DropFileWidget::ImageExtension::PNG;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("PNG (Lossless - Quality N/A)");
  } else if (index == 3) {
    m_targetExtension = DropFileWidget::ImageExtension::WEBP;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (index == 4) {
    m_targetExtension = DropFileWidget::ImageExtension::TIFF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (index == 5) {
    m_targetExtension = DropFileWidget::ImageExtension::BMP;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("BMP (Uncompressed)");
  } else if (index == 6) {
    m_targetExtension = DropFileWidget::ImageExtension::GIF;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("GIF (Indexed Color)");
  } else if (index == 7) {
    m_targetExtension = DropFileWidget::ImageExtension::PDF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("PDF Image Quality");
  }
}

bool ConvertImagePanel::execute(const QStringList &sourcePaths) {
  if (!m_controller || m_controller->isRunning()) {
    return false;
  }

  if (sourcePaths.isEmpty()) {
    QWidget *toastParent = window() ? window() : this;
    ui::Toast::show(toastParent, "No Files Selected",
                    "Please select at least one file.",
                    ui::ToastVariant::Warning);
    return false;
  }

  for (const QString &p : sourcePaths) {
    if (QFileInfo(p).suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
      QWidget *toastParent = window() ? window() : this;
      ui::Toast::show(toastParent, "PDF Input Not Supported Here",
                      "Please use PDF Tools -> PDF to Images to extract pages "
                      "from PDF files.",
                      ui::ToastVariant::Warning);
      return false;
    }
  }

  const DropFileWidget::ImageExtension targetExt = m_targetExtension;
  const QString targetFormatString =
      ImageConvertController::extensionToString(targetExt);
  const int quality = m_qualitySlider ? m_qualitySlider->getValue() : 100;

  ImageConvertController::Params params;
  params.sourcePaths = sourcePaths;
  params.quality = quality;
  params.targetExtension = targetExt;
  params.targetFormat = targetFormatString;

  // SCENARIO 1: Images -> PDF (Single or Merged Multi-Page)
  if (targetExt == DropFileWidget::ImageExtension::PDF) {
    QString suggestedName;
    if (sourcePaths.size() == 1) {
      suggestedName =
          QFileInfo(sourcePaths.first())
              .dir()
              .filePath(QFileInfo(sourcePaths.first()).baseName() + ".pdf");
    } else {
      suggestedName =
          QFileInfo(sourcePaths.first()).dir().filePath("merged_images.pdf");
    }

    const QString outputPath = QFileDialog::getSaveFileName(
        this, sourcePaths.size() == 1 ? "Save PDF" : "Save Merged PDF",
        suggestedName, "PDF Files (*.pdf)");
    if (outputPath.isEmpty()) {
      return false;
    }

    params.mode = ImageConvertController::Mode::ImagesToPdf;
    params.outputPath = outputPath;
    m_controller->start(params);
    return true;
  }

  // SCENARIO 2: Standard Image / SVG -> Image Conversion (Single & Batch)
  params.forceOpaque =
      (targetExt == DropFileWidget::JPG || targetExt == DropFileWidget::JPEG ||
       targetExt == DropFileWidget::BMP);

  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    const QString filter = targetFormatString.toUpper() + " (*." +
                           targetFormatString.toLower() + ")";
    const QString suggestedName =
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_converted");

    QString outputPathWithExt = QFileDialog::getSaveFileName(
        this, "Save Image As", suggestedName, filter);
    if (outputPathWithExt.isEmpty()) {
      return false;
    }

    const QString targetSuffix = targetFormatString.toLower();
    if (QFileInfo(outputPathWithExt).suffix().toLower() != targetSuffix) {
      outputPathWithExt += "." + targetSuffix;
    }

    params.mode = ImageConvertController::Mode::SingleImage;
    params.outputPath = outputPathWithExt;
    m_controller->start(params);
    return true;
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory", QDir::homePath());
  if (outputDir.isEmpty()) {
    return false;
  }

  params.mode = ImageConvertController::Mode::BatchImage;
  params.outputDir = outputDir;
  m_controller->start(params);
  return true;
}

void ConvertImagePanel::onConvertFinished(
    const ImageConvertController::Result &result) {
  emit executionFinished();

  QWidget *toastParent = window() ? window() : this;

  switch (result.mode) {
  case ImageConvertController::Mode::ImagesToPdf: {
    if (result.success) {
      const QString sizeStats =
          QString("\n\nTotal Original: %1\nPDF Size: %2")
              .arg(DropFileWidget::formatFileSize(result.totalOrigBytes))
              .arg(DropFileWidget::formatFileSize(result.totalOutputBytes));
      QString msg =
          (result.successCount == 1)
              ? QString("Image converted to PDF successfully!%1").arg(sizeStats)
              : QString("Successfully merged %1 images into PDF!%2")
                    .arg(result.successCount)
                    .arg(sizeStats);
      if (result.skippedCount > 0) {
        msg += QString("\n(%1 invalid image(s) skipped)")
                   .arg(result.skippedCount);
      }
      ui::Dialog::info(this, "Success", msg);
    } else if (result.cancelled) {
      ui::Toast::show(toastParent, "Cancelled", "PDF creation was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(toastParent, "Error", "Failed to create PDF!",
                      ui::ToastVariant::Destructive);
    }
    break;
  }
  case ImageConvertController::Mode::SingleImage: {
    if (result.success) {
      const QString sizeStats = formatSizeDiffStats(
          result.totalOrigBytes, result.totalOutputBytes, "Original",
          "Converted");
      ui::Dialog::info(this, "Success",
                       QString("Image converted successfully to %1%2")
                           .arg(result.outputPath)
                           .arg(sizeStats));
    } else if (result.cancelled) {
      ui::Toast::show(toastParent, "Cancelled",
                      "Image conversion was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(toastParent, "Error", result.error,
                      ui::ToastVariant::Destructive);
    }
    break;
  }
  case ImageConvertController::Mode::BatchImage: {
    const QString title = result.cancelled ? "Batch Conversion Cancelled"
                                           : "Batch Conversion Complete";
    QString message =
        QString("%1 image(s) converted successfully.\n%2 image(s) failed.")
            .arg(result.successCount)
            .arg(result.failedCount);

    if (result.successCount > 0) {
      message += formatSizeDiffStats(result.totalOrigBytes,
                                     result.totalOutputBytes, "Total Original",
                                     "Total Converted");
    }

    if (result.cancelled) {
      message += "\n(Operation was stopped early by user)";
      ui::Toast::show(toastParent, "Cancelled", message,
                      ui::ToastVariant::Default);
    } else {
      ui::Dialog::info(this, title, message);
    }
    break;
  }
  }
}
