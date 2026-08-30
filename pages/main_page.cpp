#include "main_page.h"
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
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  if (m_targetExtension) {
    onImageTargetExtensionChanged();
  }
  setLayout(mainLayout);
}

MainPage::~MainPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"jpg", "jpeg", "png", "webp", "tiff", "bmp", "gif", "pdf"};
  m_targetExtension =
      new InputWidget(this, InputType("dropdown", "Target"), extensionOptions);
  connect(m_targetExtension, &InputWidget::valueChanged, this,
          &MainPage::onImageTargetExtensionChanged);
  m_targetExtension->getValue();
}

void MainPage::setupImageAttribute() {
  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignBottom);
  attributeLayout->addWidget(m_targetExtension, 1, Qt::AlignBottom);
  mainLayout->addLayout(attributeLayout);
}

void MainPage::setupImageLayout() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_dragWidget =
      new DropFileWidget(this, "Image", m_qualitySlider, &m_targetImageExtension);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setFixedHeight(20);
  m_progressBar->setAlignment(Qt::AlignCenter);
  m_progressBar->setTextVisible(true);
  m_progressBar->setVisible(false);
  m_progressBar->setStyleSheet(
      "QProgressBar {"
      "  border: 1px solid " + Colors::Grey300.name() + ";"
      "  border-radius: 8px;"
      "  text-align: center;"
      "  background-color: " + Colors::Grey100.name() + ";"
      "  color: " + Colors::StandardBlack.name() + ";"
      "  " + TextStyle::BodySmallBold() +
      "}"
      "QProgressBar::chunk {"
      "  background-color: " + Colors::Primary500.name() + ";"
      "  border-radius: 7px;"
      "}");

  m_processButton = new ButtonAction(this, "Convert", "no");
  m_processButton->setEnabled(true);
  m_processButton->setSize(320, 42);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton = new ButtonAction(this, "Cancel", "no");
  m_cancelButton->setSize(320, 42);
  m_cancelButton->setVisible(false);
  m_cancelButton->setStyleSheet(
      "QPushButton {"
      "    padding: 12px 4px;"
      "    border: 0 solid;"
      "    border-radius: 12px;"
      "    background-color: " + Colors::Danger500.name() + ";"
      "    color: " + Colors::StandardWhite.name() + ";"
      "    " + TextStyle::BodyMediumBold() +
      "}"
      "QPushButton:hover {"
      "    background-color: " + Colors::Danger600.name() + ";"
      "}"
      "QPushButton:pressed {"
      "    background-color: " + Colors::Danger700.name() + ";"
      "}");
  connect(m_cancelButton, &QPushButton::clicked, this,
          &MainPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  setupImageAttribute();
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
}

void MainPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  m_progressBar->setVisible(isProcessing);
  m_targetExtension->setEnabled(!isProcessing);
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
    MessageBoxWidget messageBox("Error", "Please select at least one file.",
                                MessageBoxWidget::Warning, this);
    messageBox.exec();
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
    m_progressBar->setFormat(sourcePaths.size() == 1 ? "Converting to PDF... %p%"
                                                     : "Merging into PDF... %p%");

    QFuture<void> future =
        QtConcurrent::run([this, sourcePaths, outputPath, quality, totalFiles]() {
          QString tempOutputPath = outputPath + ".tmp";
          int dpi = (quality <= 0) ? 72 : (72 + static_cast<int>((quality / 100.0) * (300 - 72)));
          QPdfWriter pdfWriter(tempOutputPath);
          pdfWriter.setResolution(dpi);
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
            QImage image(path);
            if (image.isNull()) {
              skippedCount++;
              QMetaObject::invokeMethod(
                  this, [this, i]() { m_progressBar->setValue(i + 1); },
                  Qt::QueuedConnection);
              continue;
            }

            // Set page dimensions based on image aspect ratio at selected DPI (72 pt per inch)
            QSizeF ptSize(image.width() * 72.0 / dpi, image.height() * 72.0 / dpi);
            pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

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

            QRect pageRect = pdfWriter.pageLayout().paintRectPixels(dpi);
            painter.drawImage(pageRect, image);

            totalOrigBytes += QFileInfo(path).size();
            successCount++;

            QMetaObject::invokeMethod(
                this,
                [this, i]() {
                  m_progressBar->setValue(i + 1);
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
                m_cancelButton->setText("Cancel");

                if (m_isCancelled) {
                  MessageBoxWidget messageBox(
                      "Cancelled", "PDF creation was cancelled.",
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
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
                  MessageBoxWidget messageBox("Success", msg,
                                              MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else {
                  MessageBoxWidget messageBox("Error", "Failed to create PDF!",
                                              MessageBoxWidget::Critical, this);
                  messageBox.exec();
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
    m_progressBar->setFormat("Extracting PDF pages (%v of %m)... %p%");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePaths, outputDir, quality, targetExt, dragWidget]() {
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
                    this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                    Qt::QueuedConnection);
                continue;
              }

              int pageCount = doc.pageCount();
              QString baseName = QFileInfo(sourcePath).baseName();

              for (int p = 0; p < pageCount; ++p) {
                if (m_isCancelled) break;

                QSizeF ptSize = doc.pagePointSize(p);
                // Dynamic resolution (72 DPI to 600 DPI)
                int extractDpi = (quality <= 0) ? 72 : (72 + static_cast<int>((quality / 100.0) * (600 - 72)));
                qreal scale = extractDpi / 72.0;
                QSize renderSize(static_cast<int>(ptSize.width() * scale),
                                 static_cast<int>(ptSize.height() * scale));
                QImage pageImg = doc.render(p, renderSize);

                if (pageImg.isNull()) {
                  failedCount++;
                  processedItems++;
                  QMetaObject::invokeMethod(
                      this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
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
                    this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                    Qt::QueuedConnection);
              }
            } else {
              QImage image(sourcePath);
              if (image.isNull()) {
                failedCount++;
                processedItems++;
                QMetaObject::invokeMethod(
                    this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
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
                  this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                  Qt::QueuedConnection);
            }
          }

          QMetaObject::invokeMethod(
              this,
              [this, totalExtracted, failedCount, totalOutputBytes, outputDir]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel");

                if (m_isCancelled) {
                  MessageBoxWidget messageBox(
                      "Cancelled", "Extraction was cancelled.",
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else if (totalExtracted > 0) {
                  QString msg =
                      QString("Extracted %1 image(s) to:\n%2\n\nTotal Size: %3")
                          .arg(totalExtracted)
                          .arg(outputDir)
                          .arg(DropFileWidget::formatFileSize(totalOutputBytes));
                  if (failedCount > 0) {
                    msg += QString("\n(%1 failed)").arg(failedCount);
                  }
                  MessageBoxWidget messageBox("Success", msg,
                                              MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else {
                  MessageBoxWidget messageBox(
                      "Error", "Failed to extract images from PDF!",
                      MessageBoxWidget::Critical, this);
                  messageBox.exec();
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
    m_progressBar->setFormat("Converting image... %p%");

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
                m_cancelButton->setText("Cancel");

                if (m_isCancelled) {
                  MessageBoxWidget messageBox(
                      "Cancelled", "Image conversion was cancelled.",
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
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

                  MessageBoxWidget messageBox(
                      "Success",
                      QString("Image converted successfully to %1%2")
                          .arg(outputPathWithExt)
                          .arg(sizeStats),
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else {
                  MessageBoxWidget messageBox(
                      "Error",
                      QString("Failed to save image as %1")
                          .arg(targetFormatString),
                      MessageBoxWidget::Critical, this);
                  messageBox.exec();
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
    m_progressBar->setFormat(QString("Converting %v of %m images (%p%)"));

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
                [this, i]() {
                  m_progressBar->setValue(i + 1);
                },
                Qt::QueuedConnection);
          }

          QMetaObject::invokeMethod(
              this,
              [this, successCount, failureCount, totalOrigBytes,
               totalOutputBytes]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel");

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
                }

                MessageBoxWidget messageBox(title, message,
                                            MessageBoxWidget::Information, this);
                messageBox.exec();
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  }
}

void MainPage::onImageTargetExtensionChanged() {
  double value = m_targetExtension->getValue();
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