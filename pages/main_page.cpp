#include "main_page.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QBuffer>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QtConcurrent/QtConcurrent>

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)),
      m_targetImageExtension(DropFileWidget::ImageExtension::JPG) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(14);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  if (m_targetExtensionSelect) {
    onImageTargetExtensionChanged();
  }
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this, &MainPage::applyThemeStyles);
  applyThemeStyles();
}

MainPage::~MainPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"JPG", "JPEG", "PNG", "WEBP", "TIFF", "BMP", "GIF", "PDF"};
  m_targetExtensionSelect = new ui::Select(extensionOptions, this);
  m_targetExtensionField = new ui::FormField("Target Format", m_targetExtensionSelect, this);
  connect(m_targetExtensionSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onImageTargetExtensionChanged);
}

void MainPage::setupImageAttribute() {
  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_targetExtensionField, 1, Qt::AlignTop);
  mainLayout->addLayout(attributeLayout);
}

void MainPage::setupImageLayout() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_dragWidget =
      new DropFileWidget(this, "Image", m_qualitySlider, &m_targetImageExtension);

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignCenter);
  m_progressLabel->setVisible(false);

  m_progressBar = new ui::Progress(0, this);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Convert Files", ui::ButtonVariant::Primary, ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton = new ui::Button("Cancel Conversion", ui::ButtonVariant::Destructive, ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &MainPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  setupImageAttribute();
  mainLayout->addWidget(m_progressLabel);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
}

void MainPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_processButton) {
    m_processButton->setIcon(ui::Icon::get(":/icons/icons/lucide-refresh-cw.svg", c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void MainPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  if (m_progressLabel) {
    m_progressLabel->setVisible(isProcessing);
  }
  if (m_progressBar) {
    m_progressBar->setVisible(isProcessing);
  }
  m_dragWidget->setEnabled(!isProcessing);
  if (isProcessing) {
    m_qualitySlider->setEnabled(false);
  } else {
    onImageTargetExtensionChanged();
  }
}

void MainPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setText("Cancelling...");
    m_cancelButton->setEnabled(false);
  }
}

void MainPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No Files Selected", "Please select at least one file.",
                    ui::ToastVariant::Warning);
    return;
  }

  DropFileWidget::ImageExtension targetExt = m_targetImageExtension;
  QString targetFormatString = m_dragWidget->imageExtensionToString(targetExt);
  int quality = m_qualitySlider->getValue();

  // SCENARIO 1: Images -> PDF (Single or Merged Multi-Page)
  if (targetExt == DropFileWidget::ImageExtension::PDF) {
    QString suggestedName;
    if (sourcePaths.size() == 1) {
      suggestedName = QFileInfo(sourcePaths.first())
                          .dir()
                          .filePath(QFileInfo(sourcePaths.first()).baseName() + ".pdf");
    } else {
      suggestedName = QFileInfo(sourcePaths.first()).dir().filePath("merged_images.pdf");
    }

    QString outputPath = QFileDialog::getSaveFileName(
        this, sourcePaths.size() == 1 ? "Save PDF" : "Save Merged PDF",
        suggestedName, "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0);
    m_progressLabel->setText(sourcePaths.size() == 1 ? "Converting to PDF..."
                                                     : "Merging into PDF...");

    QFuture<void> future =
        QtConcurrent::run([this, sourcePaths, outputPath, quality, totalFiles]() {
          QString tempOutputPath = outputPath + ".tmp";
          int clampedQuality = qBound(1, quality, 100);
          int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
          qreal scaleFactor = 0.35 + (0.65 * clampedQuality) / 100.0;

          QPdfWriter pdfWriter(tempOutputPath);
          pdfWriter.setResolution(targetDpi);
          pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

          qint64 totalOrigBytes = 0;
          int successCount = 0;
          int skippedCount = 0;
          bool ok = true;

          QPainter painter;
          for (int i = 0; i < totalFiles; ++i) {
            if (m_isCancelled) {
              ok = false;
              break;
            }

            const QString &path = sourcePaths.at(i);
            QImage origImage(path);
            if (origImage.isNull()) {
              skippedCount++;
              QMetaObject::invokeMethod(
                  this, [this, i, totalFiles]() {
                    m_progressBar->setValue(i + 1);
                    if (totalFiles > 1) {
                      m_progressLabel->setText(
                          QString("Merging images (%1 of %2)...").arg(i + 1).arg(totalFiles));
                    }
                  },
                  Qt::QueuedConnection);
              continue;
            }

            // Preserve consistent physical page dimensions in points (at 96 base DPI)
            QSizeF ptSize(origImage.width() * 72.0 / 96.0,
                          origImage.height() * 72.0 / 96.0);
            pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

            // Downscale & compress image according to quality slider
            QImage processedImg = origImage.convertToFormat(QImage::Format_RGB888);
            if (clampedQuality < 95) {
              QSize scaledSize(
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
                break;
              }
              painter.setRenderHint(QPainter::SmoothPixmapTransform);
              painter.setRenderHint(QPainter::Antialiasing);
            } else {
              if (!pdfWriter.newPage()) {
                ok = false;
                break;
              }
            }

            QRect pageRect = pdfWriter.pageLayout().paintRectPixels(targetDpi);
            painter.drawImage(pageRect, processedImg);

            totalOrigBytes += QFileInfo(path).size();
            successCount++;

            QMetaObject::invokeMethod(
                this,
                [this, i, totalFiles]() {
                  m_progressBar->setValue(i + 1);
                  if (totalFiles > 1) {
                    m_progressLabel->setText(
                        QString("Merging images (%1 of %2)...").arg(i + 1).arg(totalFiles));
                  }
                },
                Qt::QueuedConnection);
          }

          if (painter.isActive()) {
            painter.end();
          }

          if (!ok || m_isCancelled || successCount == 0) {
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
          }

          QMetaObject::invokeMethod(
              this,
              [this, outputPath, ok, successCount, skippedCount, totalOrigBytes]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                if (m_isCancelled) {
                  ui::Toast::show(this, "Cancelled", "PDF creation was cancelled.",
                                  ui::ToastVariant::Default);
                } else if (ok && successCount > 0) {
                  qint64 outSize = QFileInfo(outputPath).size();
                  QString sizeStats =
                      QString("\n\nTotal Original: %1\nPDF Size: %2")
                          .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                          .arg(DropFileWidget::formatFileSize(outSize));
                  QString msg =
                      (successCount == 1)
                          ? QString("Image converted to PDF successfully!%1")
                                .arg(sizeStats)
                          : QString("Successfully merged %1 images into PDF!%2")
                                .arg(successCount)
                                .arg(sizeStats);
                  if (skippedCount > 0) {
                    msg += QString("\n(%1 invalid image(s) skipped)").arg(skippedCount);
                  }
                  ui::Dialog::info(this, "Success", msg);
                } else {
                  ui::Toast::show(this, "Error", "Failed to create PDF!",
                                  ui::ToastVariant::Destructive);
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 2: Input contains PDF -> Extract PDF Pages into Images
  bool hasPdfInput = false;
  int estimatedTotalPages = 0;
  for (const QString &p : sourcePaths) {
    if (QFileInfo(p).suffix().toLower() == "pdf") {
      hasPdfInput = true;
      QPdfDocument doc;
      if (doc.load(p) == QPdfDocument::Error::None) {
        estimatedTotalPages += doc.pageCount();
      } else {
        estimatedTotalPages += 1;
      }
    } else {
      estimatedTotalPages += 1;
    }
  }

  if (hasPdfInput) {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory for Extracted Images", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    m_progressBar->setRange(0, estimatedTotalPages > 0 ? estimatedTotalPages : 1);
    m_progressBar->setValue(0);
    m_progressLabel->setText("Extracting PDF pages...");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePaths, outputDir, quality, targetExt, dragWidget, estimatedTotalPages]() {
          int totalExtracted = 0;
          int failedCount = 0;
          int processedItems = 0;
          qint64 totalOutputBytes = 0;
          QSet<QString> usedBaseNames;
          QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

          for (const QString &sourcePath : sourcePaths) {
            if (m_isCancelled) break;

            if (QFileInfo(sourcePath).suffix().toLower() == "pdf") {
              QPdfDocument doc;
              if (doc.load(sourcePath) != QPdfDocument::Error::None) {
                failedCount++;
                processedItems++;
                QMetaObject::invokeMethod(
                    this,
                    [this, processedItems, estimatedTotalPages]() {
                      m_progressBar->setValue(processedItems);
                      m_progressLabel->setText(
                          QString("Extracting PDF pages (%1 of %2)...")
                              .arg(processedItems)
                              .arg(estimatedTotalPages));
                    },
                    Qt::QueuedConnection);
                continue;
              }

              int pageCount = doc.pageCount();
              QString baseName = QFileInfo(sourcePath).baseName();

              for (int p = 0; p < pageCount; ++p) {
                if (m_isCancelled) break;

                QSizeF ptSize = doc.pagePointSize(p);
                int extractDpi = (quality <= 0) ? 72 : (72 + static_cast<int>((quality / 100.0) * (600 - 72)));
                qreal scale = extractDpi / 72.0;
                QSize renderSize(qMax(1, static_cast<int>(ptSize.width() * scale)),
                                 qMax(1, static_cast<int>(ptSize.height() * scale)));
                QImage pageImg = doc.render(p, renderSize);

                if (pageImg.isNull()) {
                  failedCount++;
                  processedItems++;
                  QMetaObject::invokeMethod(
                      this,
                      [this, processedItems, estimatedTotalPages]() {
                        m_progressBar->setValue(processedItems);
                        m_progressLabel->setText(
                            QString("Extracting PDF pages (%1 of %2)...")
                                .arg(processedItems)
                                .arg(estimatedTotalPages));
                      },
                      Qt::QueuedConnection);
                  continue;
                }

                QString uniqueName = QString("%1_page%2").arg(baseName).arg(p + 1);
                int suffix = 1;
                while (usedBaseNames.contains(uniqueName.toLower()) ||
                       QFile::exists(
                           QDir(outputDir).filePath(uniqueName + "." + ext))) {
                  uniqueName =
                      QString("%1_page%2_%3").arg(baseName).arg(p + 1).arg(suffix++);
                }
                usedBaseNames.insert(uniqueName.toLower());

                QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
                if (dragWidget->saveImage(&pageImg, baseOutputPath, quality,
                                          &targetExt)) {
                  totalExtracted++;
                  totalOutputBytes +=
                      QFileInfo(baseOutputPath + "." + ext).size();
                } else {
                  failedCount++;
                }

                processedItems++;
                QMetaObject::invokeMethod(
                    this,
                    [this, processedItems, estimatedTotalPages]() {
                      m_progressBar->setValue(processedItems);
                      m_progressLabel->setText(
                          QString("Extracting PDF pages (%1 of %2)...")
                              .arg(processedItems)
                              .arg(estimatedTotalPages));
                    },
                    Qt::QueuedConnection);
              }
            } else {
              QImage image(sourcePath);
              if (image.isNull()) {
                failedCount++;
                processedItems++;
                QMetaObject::invokeMethod(
                    this,
                    [this, processedItems, estimatedTotalPages]() {
                      m_progressBar->setValue(processedItems);
                      m_progressLabel->setText(
                          QString("Extracting PDF pages (%1 of %2)...")
                              .arg(processedItems)
                              .arg(estimatedTotalPages));
                    },
                    Qt::QueuedConnection);
                continue;
              }
              QString baseName = QFileInfo(sourcePath).baseName();
              QString uniqueName = baseName;
              int suffix = 1;
              while (usedBaseNames.contains(uniqueName.toLower()) ||
                     QFile::exists(
                         QDir(outputDir).filePath(uniqueName + "." + ext))) {
                uniqueName = QString("%1_%2").arg(baseName).arg(suffix++);
              }
              usedBaseNames.insert(uniqueName.toLower());

              QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
              if (dragWidget->saveImage(&image, baseOutputPath, quality,
                                        &targetExt)) {
                totalExtracted++;
                totalOutputBytes +=
                    QFileInfo(baseOutputPath + "." + ext).size();
              } else {
                failedCount++;
              }

              processedItems++;
              QMetaObject::invokeMethod(
                  this,
                  [this, processedItems, estimatedTotalPages]() {
                    m_progressBar->setValue(processedItems);
                    m_progressLabel->setText(
                        QString("Extracting PDF pages (%1 of %2)...")
                            .arg(processedItems)
                            .arg(estimatedTotalPages));
                  },
                  Qt::QueuedConnection);
            }
          }

          QMetaObject::invokeMethod(
              this,
              [this, totalExtracted, failedCount, totalOutputBytes, outputDir]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                if (m_isCancelled) {
                  ui::Toast::show(this, "Cancelled", "Extraction was cancelled.",
                                  ui::ToastVariant::Default);
                } else if (totalExtracted > 0) {
                  QString msg =
                      QString("Extracted %1 image(s) to:\n%2\n\nTotal Size: %3")
                          .arg(totalExtracted)
                          .arg(outputDir)
                          .arg(DropFileWidget::formatFileSize(totalOutputBytes));
                  if (failedCount > 0) {
                    msg += QString("\n(%1 failed)").arg(failedCount);
                  }
                  ui::Dialog::info(this, "Success", msg);
                } else {
                  ui::Toast::show(this, "Error", "Failed to extract images from PDF!",
                                  ui::ToastVariant::Destructive);
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 3: Standard Image -> Image Conversion (Single & Batch)
  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    QString filter = targetFormatString.toUpper() + " (*." +
                     targetFormatString.toLower() + ")";
    QString suggestedName =
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_converted");

    QString outputPathWithExt = QFileDialog::getSaveFileName(
        this, "Save Image As", suggestedName, filter);

    if (outputPathWithExt.isEmpty()) {
      return;
    }

    const QString targetSuffix = targetFormatString.toLower();
    if (QFileInfo(outputPathWithExt).suffix().toLower() != targetSuffix) {
      outputPathWithExt += "." + targetSuffix;
    }

    QFileInfo outFi(outputPathWithExt);
    QString outputPathWithoutExt = outFi.dir().filePath(outFi.completeBaseName());

    setProcessingState(true);
    m_isCancelled = false;
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0);
    m_progressLabel->setText("Converting image...");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePath, outputPathWithoutExt, outputPathWithExt, quality,
         targetExt, targetFormatString, dragWidget]() {
          QImage image(sourcePath);
          bool success = false;
          if (!image.isNull() && !m_isCancelled) {
            success = dragWidget->saveImage(&image, outputPathWithoutExt,
                                            quality, &targetExt);
          }

          QMetaObject::invokeMethod(
              this,
              [this, sourcePath, success, outputPathWithExt,
               targetFormatString]() {
                m_progressBar->setValue(1);
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                if (m_isCancelled) {
                  ui::Toast::show(this, "Cancelled", "Image conversion was cancelled.",
                                  ui::ToastVariant::Default);
                } else if (success) {
                  qint64 origSize = QFileInfo(sourcePath).size();
                  qint64 outSize = QFileInfo(outputPathWithExt).size();
                  QString sizeStats;
                  if (origSize > 0 && outSize > 0) {
                    double diffPct =
                        (1.0 - (static_cast<double>(outSize) / origSize)) * 100.0;
                    sizeStats =
                        QString("\n\nOriginal: %1\nConverted: %2 (%3% %4)")
                            .arg(DropFileWidget::formatFileSize(origSize))
                            .arg(DropFileWidget::formatFileSize(outSize))
                            .arg(qAbs(diffPct), 0, 'f', 1)
                            .arg(diffPct >= 0 ? "smaller" : "larger");
                  }

                  ui::Dialog::info(
                      this, "Success",
                      QString("Image converted successfully to %1%2")
                          .arg(outputPathWithExt)
                          .arg(sizeStats));
                } else {
                  ui::Toast::show(
                      this, "Error",
                      QString("Failed to save image as %1")
                          .arg(targetFormatString),
                      ui::ToastVariant::Destructive);
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  } else {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0);
    m_progressLabel->setText(QString("Converting 0 of %1 images...").arg(totalFiles));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePaths, outputDir, quality, targetExt, totalFiles, dragWidget]() {
          int successCount = 0;
          int failureCount = 0;
          qint64 totalOrigBytes = 0;
          qint64 totalOutputBytes = 0;
          QSet<QString> usedBaseNames;
          QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

          for (int i = 0; i < totalFiles; ++i) {
            if (m_isCancelled) {
              break;
            }

            const QString &sourcePath = sourcePaths.at(i);
            QImage image(sourcePath);
            if (image.isNull()) {
              failureCount++;
            } else {
              QFileInfo fileInfo(sourcePath);
              QString baseName = fileInfo.baseName();
              QString uniqueBaseName = baseName;
              int suffix = 1;
              while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
                     QFile::exists(
                         QDir(outputDir).filePath(uniqueBaseName + "." + ext))) {
                uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
              }
              usedBaseNames.insert(uniqueBaseName.toLower());

              QString baseOutputName = QDir(outputDir).filePath(uniqueBaseName);
              if (dragWidget->saveImage(&image, baseOutputName, quality,
                                        &targetExt)) {
                successCount++;
                totalOrigBytes += fileInfo.size();
                QString outputFilePath = baseOutputName + "." + ext;
                totalOutputBytes += QFileInfo(outputFilePath).size();
              } else {
                failureCount++;
              }
            }

            QMetaObject::invokeMethod(
                this,
                [this, i, totalFiles]() {
                  m_progressBar->setValue(i + 1);
                  m_progressLabel->setText(
                      QString("Converting %1 of %2 images...").arg(i + 1).arg(totalFiles));
                },
                Qt::QueuedConnection);
          }

          QMetaObject::invokeMethod(
              this,
              [this, successCount, failureCount, totalOrigBytes,
               totalOutputBytes]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                QString title = m_isCancelled ? "Batch Conversion Cancelled"
                                              : "Batch Conversion Complete";
                QString message =
                    QString("%1 image(s) converted successfully.\n%2 image(s) failed.")
                        .arg(successCount)
                        .arg(failureCount);

                if (totalOrigBytes > 0 && totalOutputBytes > 0 &&
                    successCount > 0) {
                  double diffPct =
                      (1.0 - (static_cast<double>(totalOutputBytes) /
                              totalOrigBytes)) *
                      100.0;
                  message +=
                      QString("\n\nTotal Original: %1\nTotal Converted: %2 (%3% %4)")
                          .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                          .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                          .arg(qAbs(diffPct), 0, 'f', 1)
                          .arg(diffPct >= 0 ? "smaller" : "larger");
                }

                if (m_isCancelled) {
                  message += "\n(Operation was stopped early by user)";
                  ui::Toast::show(this, "Cancelled", message, ui::ToastVariant::Default);
                } else {
                  ui::Dialog::info(this, title, message);
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  }
}

void MainPage::onImageTargetExtensionChanged() {
  int value = m_targetExtensionSelect ? m_targetExtensionSelect->currentIndex() : 0;
  if (value == 0) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 1) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPEG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 2) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PNG;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("PNG (Lossless - Quality N/A)");
  } else if (value == 3) {
    m_targetImageExtension = DropFileWidget::ImageExtension::WEBP;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 4) {
    m_targetImageExtension = DropFileWidget::ImageExtension::TIFF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 5) {
    m_targetImageExtension = DropFileWidget::ImageExtension::BMP;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("BMP (Uncompressed)");
  } else if (value == 6) {
    m_targetImageExtension = DropFileWidget::ImageExtension::GIF;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("GIF (Indexed Color)");
  } else if (value == 7) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PDF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("PDF Image Quality");
  }
}