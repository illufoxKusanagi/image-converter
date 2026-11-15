#include "convert_image_panel.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QBuffer>
#include <QByteArray>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFuture>
#include <QHBoxLayout>
#include <QIODevice>
#include <QImageReader>
#include <QMarginsF>
#include <QMetaObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QRect>
#include <QRectF>
#include <QSet>
#include <QSize>
#include <QSizeF>
#include <QSizePolicy>
#include <QSvgRenderer>
#include <QtConcurrent/QtConcurrent>
#include <QtGlobal>
#include <memory>
#include <utility>

namespace {

QString resolveUniqueBaseName(const QString &baseName, const QString &outputDir,
                              const QString &ext,
                              QSet<QString> &usedBaseNames) {
  QString uniqueName = baseName;
  int suffix = 1;
  while (usedBaseNames.contains(uniqueName.toLower()) ||
         QFile::exists(QDir(outputDir).filePath(uniqueName + "." + ext))) {
    uniqueName = QString("%1_%2").arg(baseName).arg(suffix++);
  }
  usedBaseNames.insert(uniqueName.toLower());
  return uniqueName;
}

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
    : QWidget(parent), m_futureWatcher(new QFutureWatcher<void>(this)) {
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
  connect(m_futureWatcher, &QFutureWatcher<void>::finished, this,
          &ConvertImagePanel::onFutureFinished);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_targetExtensionField, 1, Qt::AlignTop);

  onExtensionChanged(m_targetExtensionSelect->currentIndex());
}

ConvertImagePanel::~ConvertImagePanel() {
  m_pendingCompletion = nullptr;
  cancel();
  if (m_futureWatcher) {
    m_futureWatcher->disconnect(this);
    if (m_futureWatcher->isRunning()) {
      m_futureWatcher->waitForFinished();
    }
  }
}

QImage ConvertImagePanel::loadRasterOrSvgImage(const QString &path,
                                               bool forceOpaqueWhite) {
  const QFileInfo fi(path);
  if (fi.suffix().compare("svg", Qt::CaseInsensitive) == 0) {
    QSvgRenderer renderer(fi.absoluteFilePath());
    if (!renderer.isValid()) {
      return QImage();
    }
    QSize baseSize = renderer.defaultSize();
    if (!baseSize.isValid() || baseSize.width() <= 0 ||
        baseSize.height() <= 0) {
      baseSize = renderer.viewBoxF().size().toSize();
    }
    if (!baseSize.isValid() || baseSize.width() <= 0 ||
        baseSize.height() <= 0) {
      baseSize = QSize(512, 512);
    }
    if (baseSize.width() > 16384 || baseSize.height() > 16384) {
      baseSize.scale(16384, 16384, Qt::KeepAspectRatio);
    }
    baseSize.setWidth(qBound(1, baseSize.width(), 16384));
    baseSize.setHeight(qBound(1, baseSize.height(), 16384));

    QImage img(baseSize, forceOpaqueWhite
                             ? QImage::Format_RGB32
                             : QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) {
      return QImage();
    }
    img.fill(forceOpaqueWhite ? Qt::white : Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter,
                    QRectF(0.0, 0.0, baseSize.width(), baseSize.height()));
    painter.end();
    return img;
  }

  QImageReader reader(fi.absoluteFilePath());
  reader.setAutoTransform(true);
  QImage img = reader.read();
  if (img.isNull()) {
    img = QImage(fi.absoluteFilePath());
  }
  if (!img.isNull() && forceOpaqueWhite && img.hasAlphaChannel()) {
    QImage opaque(img.size(), QImage::Format_RGB32);
    if (!opaque.isNull()) {
      opaque.fill(Qt::white);
      QPainter painter(&opaque);
      painter.drawImage(0, 0, img);
      painter.end();
      return opaque;
    }
  }
  return img;
}

SliderWidget *ConvertImagePanel::qualitySlider() const {
  return m_qualitySlider;
}

DropFileWidget::ImageExtension *ConvertImagePanel::targetExtensionPtr() {
  return &m_targetExtension;
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

void ConvertImagePanel::cancel() {
  m_isCancelled.store(true);
  if (m_futureWatcher && m_futureWatcher->isRunning()) {
    m_futureWatcher->cancel();
  }
}

bool ConvertImagePanel::isRunning() const {
  return m_futureWatcher && m_futureWatcher->isRunning();
}

void ConvertImagePanel::onFutureFinished() {
  auto completion = std::move(m_pendingCompletion);
  m_pendingCompletion = nullptr;
  if (completion) {
    completion();
  }
}

void ConvertImagePanel::postProgress(int completed, int total,
                                     const QString &labelText) {
  QMetaObject::invokeMethod(
      this,
      [this, completed, total, labelText]() {
        emit progressUpdated(completed, total, labelText);
      },
      Qt::QueuedConnection);
}

void ConvertImagePanel::postFileStatus(
    DropFileWidget *dragWidget, const QPointer<DropFileWidget> &safeDragWidget,
    const QString &path, DropFileWidget::FileStatus status,
    const QString &detail) {
  if (!dragWidget) {
    return;
  }
  QMetaObject::invokeMethod(
      dragWidget,
      [safeDragWidget, path, status, detail]() {
        if (safeDragWidget) {
          safeDragWidget->setFileStatus(path, status, detail);
        }
      },
      Qt::QueuedConnection);
}

bool ConvertImagePanel::execute(DropFileWidget *dragWidget) {
  if (!dragWidget || isRunning()) {
    return false;
  }

  const QStringList sourcePaths = dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    QWidget *toastParent = window() ? window() : this;
    ui::Toast::show(toastParent, "No Files Selected",
                    "Please select at least one file.",
                    ui::ToastVariant::Warning);
    return false;
  }

  const DropFileWidget::ImageExtension targetExt = m_targetExtension;
  const QString targetFormatString =
      dragWidget->imageExtensionToString(targetExt);
  const int quality = m_qualitySlider ? m_qualitySlider->getValue() : 100;

  // SCENARIO 1: Images -> PDF (Single or Merged Multi-Page)
  if (targetExt == DropFileWidget::ImageExtension::PDF) {
    return executeImagesToPdf(dragWidget, sourcePaths, quality);
  }

  // SCENARIO 2: Input contains PDF -> Extract PDF Pages into Images
  bool hasPdfInput = false;
  for (const QString &p : sourcePaths) {
    if (QFileInfo(p).suffix().toLower() == "pdf") {
      hasPdfInput = true;
      break;
    }
  }
  if (hasPdfInput) {
    return executePdfToImages(dragWidget, sourcePaths, quality, targetExt,
                              targetFormatString);
  }

  // SCENARIO 3: Standard Image / SVG -> Image Conversion (Single & Batch)
  const bool forceOpaque =
      (targetExt == DropFileWidget::JPG || targetExt == DropFileWidget::JPEG ||
       targetExt == DropFileWidget::BMP);

  if (sourcePaths.size() == 1) {
    return executeSingleImageConvert(dragWidget, sourcePaths.first(), quality,
                                     targetExt, targetFormatString,
                                     forceOpaque);
  }

  return executeBatchImageConvert(dragWidget, sourcePaths, quality, targetExt,
                                  targetFormatString, forceOpaque);
}

bool ConvertImagePanel::executeImagesToPdf(DropFileWidget *dragWidget,
                                           const QStringList &sourcePaths,
                                           int quality) {
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

  m_isCancelled.store(false);
  const int totalFiles = sourcePaths.size();
  const QString initialText =
      QString("Adding %1 to PDF (1 of %2)...")
          .arg(QFileInfo(sourcePaths.first()).fileName())
          .arg(totalFiles);

  struct PdfResultState {
    bool saved = false;
    bool cancelled = false;
    int successCount = 0;
    int skippedCount = 0;
    int startedFiles = 0;
    qint64 totalOrigBytes = 0;
    QStringList includedPaths;
    QString failedPath;
  };

  auto state = std::make_shared<PdfResultState>();
  const QPointer<DropFileWidget> safeDragWidget(dragWidget);
  using FileStatus = DropFileWidget::FileStatus;

  m_pendingCompletion = [this, safeDragWidget, sourcePaths, outputPath,
                         state]() {
    if (safeDragWidget) {
      for (const QString &path : state->includedPaths) {
        safeDragWidget->setFileStatus(
            path,
            state->saved ? FileStatus::Succeeded
                         : (state->cancelled ? FileStatus::Cancelled
                                             : FileStatus::Failed),
            state->saved
                ? QString("Saved in %1").arg(QFileInfo(outputPath).fileName())
                : (state->cancelled ? "PDF creation cancelled; no output saved"
                                    : "Failed to save PDF"));
      }
      if (!state->failedPath.isEmpty()) {
        safeDragWidget->setFileStatus(state->failedPath, FileStatus::Failed,
                                      "Failed to add image to PDF");
      }
      for (int i = state->startedFiles; i < sourcePaths.size(); ++i) {
        safeDragWidget->setFileStatus(
            sourcePaths.at(i),
            state->cancelled ? FileStatus::Cancelled : FileStatus::Failed,
            state->cancelled ? "Not processed (cancelled)"
                             : "Not processed: PDF creation failed");
      }
    }

    emit executionFinished();

    QWidget *toastParent = window() ? window() : this;
    if (state->saved) {
      const qint64 outSize = QFileInfo(outputPath).size();
      const QString sizeStats =
          QString("\n\nTotal Original: %1\nPDF Size: %2")
              .arg(DropFileWidget::formatFileSize(state->totalOrigBytes))
              .arg(DropFileWidget::formatFileSize(outSize));
      QString msg =
          (state->successCount == 1)
              ? QString("Image converted to PDF successfully!%1").arg(sizeStats)
              : QString("Successfully merged %1 images into PDF!%2")
                    .arg(state->successCount)
                    .arg(sizeStats);
      if (state->skippedCount > 0) {
        msg += QString("\n(%1 invalid image(s) skipped)")
                   .arg(state->skippedCount);
      }
      ui::Dialog::info(this, "Success", msg);
    } else if (state->cancelled) {
      ui::Toast::show(toastParent, "Cancelled", "PDF creation was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(toastParent, "Error", "Failed to create PDF!",
                      ui::ToastVariant::Destructive);
    }
  };

  QFuture<void> future =
      QtConcurrent::run([this, dragWidget, safeDragWidget, sourcePaths,
                         outputPath, quality, totalFiles, state]() {
        const QString tempOutputPath = outputPath + ".tmp";
        const int clampedQuality = qBound(1, quality, 100);
        const int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
        const qreal scaleFactor = 0.35 + (0.65 * clampedQuality) / 100.0;

        QPdfWriter pdfWriter(tempOutputPath);
        pdfWriter.setResolution(targetDpi);
        pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

        qint64 totalOrigBytes = 0;
        int successCount = 0;
        int skippedCount = 0;
        int startedFiles = 0;
        QStringList includedPaths;
        QString failedPath;
        bool ok = true;

        QPainter painter;
        for (int i = 0; i < totalFiles; ++i) {
          if (m_isCancelled.load()) {
            ok = false;
            break;
          }

          const QString &path = sourcePaths.at(i);
          startedFiles++;
          postFileStatus(dragWidget, safeDragWidget, path,
                         FileStatus::Processing, "Adding image to PDF");
          postProgress(i, totalFiles,
                       QString("Adding %1 to PDF (%2 of %3)...")
                           .arg(QFileInfo(path).fileName())
                           .arg(i + 1)
                           .arg(totalFiles));

          const QImage origImage = loadRasterOrSvgImage(path, true);
          if (origImage.isNull()) {
            skippedCount++;
            postFileStatus(dragWidget, safeDragWidget, path, FileStatus::Failed,
                           "Could not load image");
            postProgress(i + 1, totalFiles, QString());
            continue;
          }

          // Preserve consistent physical page dimensions in points (at 96 base
          // DPI)
          const QSizeF ptSize(origImage.width() * 72.0 / 96.0,
                              origImage.height() * 72.0 / 96.0);
          pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

          // Downscale & compress image according to quality slider
          QImage processedImg =
              origImage.convertToFormat(QImage::Format_RGB888);
          if (clampedQuality < 95) {
            const QSize scaledSize(
                qMax(1, qRound(origImage.width() * scaleFactor)),
                qMax(1, qRound(origImage.height() * scaleFactor)));
            processedImg = processedImg.scaled(scaledSize, Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
            QByteArray jpegBytes;
            QBuffer buffer(&jpegBytes);
            if (buffer.open(QIODevice::WriteOnly)) {
              processedImg.save(&buffer, "JPG", clampedQuality);
              buffer.close();
              QImage reloaded;
              if (reloaded.loadFromData(jpegBytes, "JPG")) {
                processedImg = reloaded;
              }
            }
          }

          if (successCount == 0) {
            if (!painter.begin(&pdfWriter)) {
              ok = false;
              failedPath = path;
              break;
            }
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.setRenderHint(QPainter::Antialiasing);
          } else {
            if (!pdfWriter.newPage()) {
              ok = false;
              failedPath = path;
              break;
            }
          }

          const QRect pageRect =
              pdfWriter.pageLayout().paintRectPixels(targetDpi);
          painter.drawImage(pageRect, processedImg);

          totalOrigBytes += QFileInfo(path).size();
          successCount++;
          includedPaths.append(path);
          postFileStatus(dragWidget, safeDragWidget, path, FileStatus::Waiting,
                         "Added; awaiting PDF save");
          postProgress(i + 1, totalFiles, QString());
        }

        if (ok && !m_isCancelled.load() && successCount > 0) {
          postProgress(startedFiles, totalFiles,
                       QString("Saving %1 (%2 image(s))...")
                           .arg(QFileInfo(outputPath).fileName())
                           .arg(successCount));
        }
        if (painter.isActive() && !painter.end()) {
          ok = false;
        }

        const bool cancelled = m_isCancelled.load();
        bool saved = false;
        if (!ok || cancelled || successCount == 0) {
          QFile::remove(tempOutputPath);
        } else {
          const bool hadExisting = QFile::exists(outputPath);
          const QString backupPath = outputPath + ".bak";
          if (hadExisting) {
            QFile::remove(backupPath);
            if (!QFile::rename(outputPath, backupPath)) {
              QFile::remove(tempOutputPath);
              ok = false;
            }
          }
          if (ok && !QFile::rename(tempOutputPath, outputPath)) {
            QFile::remove(tempOutputPath);
            if (hadExisting) {
              QFile::rename(backupPath, outputPath);
            }
            ok = false;
          }
          if (ok && hadExisting) {
            QFile::remove(backupPath);
          }
          saved = ok;
        }

        state->saved = saved;
        state->cancelled = cancelled;
        state->successCount = successCount;
        state->skippedCount = skippedCount;
        state->startedFiles = startedFiles;
        state->totalOrigBytes = totalOrigBytes;
        state->includedPaths = includedPaths;
        state->failedPath = failedPath;
      });

  emit progressConfigured(0, totalFiles, totalFiles == 1, initialText);
  m_futureWatcher->setFuture(future);
  return true;
}

bool ConvertImagePanel::executePdfToImages(
    DropFileWidget *dragWidget, const QStringList &sourcePaths, int quality,
    DropFileWidget::ImageExtension targetExt,
    const QString &targetFormatString) {
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Extracted Images", QDir::homePath());
  if (outputDir.isEmpty()) {
    return false;
  }

  m_isCancelled.store(false);
  const int totalFiles = sourcePaths.size();
  const QString ext = targetFormatString.toLower();
  const QString initialText =
      QString("Processing %1 (file 1 of %2)...")
          .arg(QFileInfo(sourcePaths.first()).fileName())
          .arg(totalFiles);

  struct ExtractResultState {
    int totalExtracted = 0;
    int failedCount = 0;
    int startedFiles = 0;
    qint64 totalOutputBytes = 0;
    bool cancelled = false;
  };

  auto state = std::make_shared<ExtractResultState>();
  const QPointer<DropFileWidget> safeDragWidget(dragWidget);
  using FileStatus = DropFileWidget::FileStatus;

  m_pendingCompletion = [this, safeDragWidget, sourcePaths, outputDir,
                         state]() {
    if (safeDragWidget) {
      for (int i = state->startedFiles; i < sourcePaths.size(); ++i) {
        safeDragWidget->setFileStatus(sourcePaths.at(i), FileStatus::Cancelled,
                                      "Not processed (cancelled)");
      }
    }

    emit executionFinished();

    QWidget *toastParent = window() ? window() : this;
    if (state->cancelled) {
      ui::Toast::show(toastParent, "Cancelled", "Extraction was cancelled.",
                      ui::ToastVariant::Default);
    } else if (state->totalExtracted > 0) {
      QString msg =
          QString("Extracted %1 image(s) to:\n%2\n\nTotal Size: %3")
              .arg(state->totalExtracted)
              .arg(outputDir)
              .arg(DropFileWidget::formatFileSize(state->totalOutputBytes));
      if (state->failedCount > 0) {
        msg += QString("\n(%1 failed)").arg(state->failedCount);
      }
      ui::Dialog::info(this, "Success", msg);
    } else {
      ui::Toast::show(toastParent, "Error",
                      "Failed to extract images from PDF!",
                      ui::ToastVariant::Destructive);
    }
  };

  QFuture<void> future =
      QtConcurrent::run([this, dragWidget, safeDragWidget, sourcePaths,
                         outputDir, quality, targetExt, ext, totalFiles,
                         state]() {
        int totalExtracted = 0;
        int failedCount = 0;
        int completedFiles = 0;
        int startedFiles = 0;
        qint64 totalOutputBytes = 0;
        QSet<QString> usedBaseNames;
        const bool forceOpaque = (targetExt == DropFileWidget::JPG ||
                                  targetExt == DropFileWidget::JPEG ||
                                  targetExt == DropFileWidget::BMP);

        for (int i = 0; i < sourcePaths.size(); ++i) {
          if (m_isCancelled.load()) {
            break;
          }

          const QString &sourcePath = sourcePaths.at(i);
          startedFiles++;
          postFileStatus(dragWidget, safeDragWidget, sourcePath,
                         FileStatus::Processing, "Loading source");
          postProgress(completedFiles, totalFiles,
                       QString("Processing %1 (file %2 of %3)...")
                           .arg(QFileInfo(sourcePath).fileName())
                           .arg(i + 1)
                           .arg(sourcePaths.size()));

          if (QFileInfo(sourcePath).suffix().toLower() == "pdf") {
            QPdfDocument doc;
            if (doc.load(sourcePath) != QPdfDocument::Error::None) {
              failedCount++;
              completedFiles++;
              postFileStatus(dragWidget, safeDragWidget, sourcePath,
                             FileStatus::Failed, "Could not load PDF");
              if (totalFiles > 1) {
                postProgress(completedFiles, totalFiles, QString());
              }
              continue;
            }

            const int pageCount = doc.pageCount();
            if (pageCount <= 0) {
              failedCount++;
              completedFiles++;
              postFileStatus(dragWidget, safeDragWidget, sourcePath,
                             FileStatus::Failed, "PDF contains no pages");
              if (totalFiles > 1) {
                postProgress(completedFiles, totalFiles, QString());
              }
              continue;
            }

            if (totalFiles == 1) {
              const QString fileLabel =
                  QString("Processing %1 (file 1 of 1)...")
                      .arg(QFileInfo(sourcePath).fileName());
              QMetaObject::invokeMethod(
                  this,
                  [this, pageCount, fileLabel]() {
                    emit progressConfigured(0, pageCount, false, fileLabel);
                  },
                  Qt::QueuedConnection);
            }

            const auto updatePageProgress = [this, totalFiles, pageCount,
                                             &completedFiles](
                                                int completedPages,
                                                const QString &label) {
              postProgress(totalFiles == 1 ? completedPages : completedFiles,
                           totalFiles == 1 ? pageCount : totalFiles, label);
            };

            int completedPages = 0;
            int extractedPages = 0;
            int failedPages = 0;
            const QString baseName = QFileInfo(sourcePath).baseName();

            for (int p = 0; p < pageCount; ++p) {
              if (m_isCancelled.load()) {
                break;
              }

              postFileStatus(
                  dragWidget, safeDragWidget, sourcePath,
                  FileStatus::Processing,
                  QString("Page %1 of %2").arg(p + 1).arg(pageCount));
              updatePageProgress(
                  completedPages,
                  QString("Extracting %1 (file %2 of %3, page %4 of %5)...")
                      .arg(QFileInfo(sourcePath).fileName())
                      .arg(i + 1)
                      .arg(sourcePaths.size())
                      .arg(p + 1)
                      .arg(pageCount));

              const QSizeF ptSize = doc.pagePointSize(p);
              const int extractDpi =
                  (quality <= 0)
                      ? 72
                      : (72 + static_cast<int>((quality / 100.0) * (600 - 72)));
              const qreal scale = extractDpi / 72.0;
              const QSize renderSize(
                  qMax(1, static_cast<int>(ptSize.width() * scale)),
                  qMax(1, static_cast<int>(ptSize.height() * scale)));
              QImage pageImg = doc.render(p, renderSize);

              if (pageImg.isNull()) {
                failedCount++;
                failedPages++;
                completedPages++;
                updatePageProgress(completedPages, QString());
                continue;
              }

              if (pageImg.hasAlphaChannel()) {
                QImage opaquePage(pageImg.size(), QImage::Format_RGB32);
                if (opaquePage.isNull()) {
                  failedCount++;
                  failedPages++;
                  completedPages++;
                  updatePageProgress(completedPages, QString());
                  continue;
                }
                opaquePage.fill(Qt::white);
                opaquePage.setDotsPerMeterX(pageImg.dotsPerMeterX());
                opaquePage.setDotsPerMeterY(pageImg.dotsPerMeterY());
                {
                  QPainter painter(&opaquePage);
                  painter.drawImage(0, 0, pageImg);
                }
                pageImg = opaquePage;
              }

              const QString pageStem =
                  QString("%1_page%2").arg(baseName).arg(p + 1);
              const QString uniqueName = resolveUniqueBaseName(
                  pageStem, outputDir, ext, usedBaseNames);

              if (m_isCancelled.load()) {
                break;
              }

              const QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
              if (dragWidget->saveImage(&pageImg, baseOutputPath, quality,
                                        &targetExt)) {
                totalExtracted++;
                extractedPages++;
                totalOutputBytes +=
                    QFileInfo(baseOutputPath + "." + ext).size();
              } else {
                failedCount++;
                failedPages++;
              }

              completedPages++;
              updatePageProgress(completedPages, QString());
            }

            if (extractedPages + failedPages < pageCount) {
              postFileStatus(
                  dragWidget, safeDragWidget, sourcePath, FileStatus::Cancelled,
                  QString("Cancelled: %1 of %2 pages saved; %3 failed")
                      .arg(extractedPages)
                      .arg(pageCount)
                      .arg(failedPages));
              break;
            } else if (failedPages > 0) {
              postFileStatus(dragWidget, safeDragWidget, sourcePath,
                             FileStatus::Failed,
                             QString("%1 of %2 pages saved; %3 failed")
                                 .arg(extractedPages)
                                 .arg(pageCount)
                                 .arg(failedPages));
            } else {
              postFileStatus(dragWidget, safeDragWidget, sourcePath,
                             FileStatus::Succeeded,
                             QString("Saved %1 page(s)").arg(extractedPages));
            }
          } else {
            const QImage image = loadRasterOrSvgImage(sourcePath, forceOpaque);
            FileStatus status = FileStatus::Failed;
            QString detail = "Could not load image";
            if (image.isNull()) {
              failedCount++;
            } else {
              const QString baseName = QFileInfo(sourcePath).baseName();
              const QString uniqueName = resolveUniqueBaseName(
                  baseName, outputDir, ext, usedBaseNames);

              const QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
              if (m_isCancelled.load()) {
                status = FileStatus::Cancelled;
                detail = "Cancelled before save";
              } else if (dragWidget->saveImage(&image, baseOutputPath, quality,
                                               &targetExt)) {
                totalExtracted++;
                totalOutputBytes +=
                    QFileInfo(baseOutputPath + "." + ext).size();
                status = FileStatus::Succeeded;
                detail = QString("Saved as %1.%2").arg(uniqueName).arg(ext);
              } else {
                failedCount++;
                detail = QString("Failed to save as %1").arg(ext.toUpper());
              }
            }
            postFileStatus(dragWidget, safeDragWidget, sourcePath, status,
                           detail);
            if (status == FileStatus::Cancelled) {
              break;
            }
          }

          completedFiles++;
          if (totalFiles > 1) {
            postProgress(completedFiles, totalFiles, QString());
          }
        }

        state->totalExtracted = totalExtracted;
        state->failedCount = failedCount;
        state->startedFiles = startedFiles;
        state->totalOutputBytes = totalOutputBytes;
        state->cancelled = m_isCancelled.load() && completedFiles < totalFiles;
      });

  emit progressConfigured(0, totalFiles, totalFiles == 1, initialText);
  m_futureWatcher->setFuture(future);
  return true;
}

bool ConvertImagePanel::executeSingleImageConvert(
    DropFileWidget *dragWidget, const QString &sourcePath, int quality,
    DropFileWidget::ImageExtension targetExt, const QString &targetFormatString,
    bool forceOpaque) {
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

  const QFileInfo outFi(outputPathWithExt);
  const QString outputPathWithoutExt =
      outFi.dir().filePath(outFi.completeBaseName());

  m_isCancelled.store(false);
  const QString initialText =
      QString("Converting %1 (1 of 1)...").arg(QFileInfo(sourcePath).fileName());

  using FileStatus = DropFileWidget::FileStatus;
  struct SingleResultState {
    FileStatus status = FileStatus::Failed;
    QString detail;
  };

  auto state = std::make_shared<SingleResultState>();
  const QPointer<DropFileWidget> safeDragWidget(dragWidget);

  m_pendingCompletion = [this, safeDragWidget, sourcePath, outputPathWithExt,
                         state]() {
    if (safeDragWidget) {
      safeDragWidget->setFileStatus(sourcePath, state->status, state->detail);
    }
    emit progressUpdated(state->status == FileStatus::Cancelled ? 0 : 1, 1,
                         QString());
    emit executionFinished();

    QWidget *toastParent = window() ? window() : this;
    if (state->status == FileStatus::Succeeded) {
      const qint64 origSize = QFileInfo(sourcePath).size();
      const qint64 outSize = QFileInfo(outputPathWithExt).size();
      const QString sizeStats =
          formatSizeDiffStats(origSize, outSize, "Original", "Converted");
      ui::Dialog::info(this, "Success",
                       QString("Image converted successfully to %1%2")
                           .arg(outputPathWithExt)
                           .arg(sizeStats));
    } else if (state->status == FileStatus::Cancelled) {
      ui::Toast::show(toastParent, "Cancelled",
                      "Image conversion was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(toastParent, "Error", state->detail,
                      ui::ToastVariant::Destructive);
    }
  };

  QFuture<void> future =
      QtConcurrent::run([this, dragWidget, safeDragWidget, sourcePath,
                         outputPathWithoutExt, outputPathWithExt, quality,
                         targetExt, targetFormatString, forceOpaque, state]() {
        postFileStatus(dragWidget, safeDragWidget, sourcePath,
                       FileStatus::Processing, "Converting image");

        const QImage image = loadRasterOrSvgImage(sourcePath, forceOpaque);
        FileStatus status = FileStatus::Failed;
        QString detail;
        if (image.isNull()) {
          detail = "Could not load image";
        } else if (m_isCancelled.load()) {
          status = FileStatus::Cancelled;
          detail = "Cancelled before save";
        } else if (dragWidget->saveImage(&image, outputPathWithoutExt, quality,
                                         &targetExt)) {
          status = FileStatus::Succeeded;
          detail = QString("Saved as %1")
                       .arg(QFileInfo(outputPathWithExt).fileName());
        } else {
          detail = QString("Failed to save image as %1").arg(targetFormatString);
        }

        state->status = status;
        state->detail = detail;
      });

  emit progressConfigured(0, 1, true, initialText);
  m_futureWatcher->setFuture(future);
  return true;
}

bool ConvertImagePanel::executeBatchImageConvert(
    DropFileWidget *dragWidget, const QStringList &sourcePaths, int quality,
    DropFileWidget::ImageExtension targetExt, const QString &targetFormatString,
    bool forceOpaque) {
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory", QDir::homePath());
  if (outputDir.isEmpty()) {
    return false;
  }

  m_isCancelled.store(false);
  const int totalFiles = sourcePaths.size();
  const QString ext = targetFormatString.toLower();
  const QString initialText =
      QString("Converting %1 (1 of %2)...")
          .arg(QFileInfo(sourcePaths.first()).fileName())
          .arg(totalFiles);

  struct BatchResultState {
    int completedFiles = 0;
    int successCount = 0;
    int failureCount = 0;
    qint64 totalOrigBytes = 0;
    qint64 totalOutputBytes = 0;
    bool cancelled = false;
  };

  auto state = std::make_shared<BatchResultState>();
  const QPointer<DropFileWidget> safeDragWidget(dragWidget);
  using FileStatus = DropFileWidget::FileStatus;

  m_pendingCompletion = [this, safeDragWidget, sourcePaths, state]() {
    if (safeDragWidget) {
      for (int i = state->completedFiles; i < sourcePaths.size(); ++i) {
        safeDragWidget->setFileStatus(sourcePaths.at(i), FileStatus::Cancelled,
                                      "Not processed (cancelled)");
      }
    }

    emit executionFinished();

    const QString title = state->cancelled ? "Batch Conversion Cancelled"
                                           : "Batch Conversion Complete";
    QString message =
        QString("%1 image(s) converted successfully.\n%2 image(s) failed.")
            .arg(state->successCount)
            .arg(state->failureCount);

    if (state->successCount > 0) {
      message += formatSizeDiffStats(state->totalOrigBytes,
                                     state->totalOutputBytes, "Total Original",
                                     "Total Converted");
    }

    QWidget *toastParent = window() ? window() : this;
    if (state->cancelled) {
      message += "\n(Operation was stopped early by user)";
      ui::Toast::show(toastParent, "Cancelled", message,
                      ui::ToastVariant::Default);
    } else {
      ui::Dialog::info(this, title, message);
    }
  };

  QFuture<void> future =
      QtConcurrent::run([this, dragWidget, safeDragWidget, sourcePaths,
                         outputDir, quality, targetExt, ext, totalFiles,
                         forceOpaque, state]() {
        int successCount = 0;
        int failureCount = 0;
        int completedFiles = 0;
        qint64 totalOrigBytes = 0;
        qint64 totalOutputBytes = 0;
        QSet<QString> usedBaseNames;

        for (int i = 0; i < totalFiles; ++i) {
          if (m_isCancelled.load()) {
            break;
          }

          const QString &sourcePath = sourcePaths.at(i);
          postFileStatus(dragWidget, safeDragWidget, sourcePath,
                         FileStatus::Processing, "Converting image");
          postProgress(completedFiles, totalFiles,
                       QString("Converting %1 (%2 of %3)...")
                           .arg(QFileInfo(sourcePath).fileName())
                           .arg(i + 1)
                           .arg(totalFiles));

          const QImage image = loadRasterOrSvgImage(sourcePath, forceOpaque);
          FileStatus status = FileStatus::Failed;
          QString detail = "Could not load image";
          if (image.isNull()) {
            failureCount++;
          } else {
            const QFileInfo fileInfo(sourcePath);
            const QString uniqueBaseName = resolveUniqueBaseName(
                fileInfo.baseName(), outputDir, ext, usedBaseNames);

            if (m_isCancelled.load()) {
              break;
            }

            const QString baseOutputName =
                QDir(outputDir).filePath(uniqueBaseName);
            if (dragWidget->saveImage(&image, baseOutputName, quality,
                                      &targetExt)) {
              successCount++;
              totalOrigBytes += fileInfo.size();
              const QString outputFilePath = baseOutputName + "." + ext;
              totalOutputBytes += QFileInfo(outputFilePath).size();
              status = FileStatus::Succeeded;
              detail = QString("Saved as %1.%2").arg(uniqueBaseName).arg(ext);
            } else {
              failureCount++;
              detail = QString("Failed to save as %1").arg(ext.toUpper());
            }
          }

          completedFiles++;
          postFileStatus(dragWidget, safeDragWidget, sourcePath, status,
                         detail);
          postProgress(completedFiles, totalFiles, QString());
        }

        state->completedFiles = completedFiles;
        state->successCount = successCount;
        state->failureCount = failureCount;
        state->totalOrigBytes = totalOrigBytes;
        state->totalOutputBytes = totalOutputBytes;
        state->cancelled = m_isCancelled.load() && completedFiles < totalFiles;
      });

  emit progressConfigured(0, totalFiles, false, initialText);
  m_futureWatcher->setFuture(future);
  return true;
}
