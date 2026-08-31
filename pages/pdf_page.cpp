#include "pdf_page.h"
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

PdfPage::PdfPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)) {
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  m_dragWidget = new DropFileWidget(this, "PDF");
  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  QStringList presetOptions = {
      "Custom", "Web / Email (70-80% smaller)", "Balanced (40-50% smaller)",
      "High Quality (15-25% smaller)"};
  m_presetDropdown =
      new InputWidget(this, InputType("dropdown", "Preset"), presetOptions);
  connect(m_presetDropdown, &InputWidget::valueChanged, this,
          &PdfPage::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfPage::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignBottom);
  attributeLayout->addWidget(m_presetDropdown, 1, Qt::AlignBottom);

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

  m_processButton = new ButtonAction(this, "Compress PDF", "no");
  m_processButton->setEnabled(true);
  m_processButton->setSize(320, 42);
  connect(m_processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);

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
          &PdfPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  mainLayout->addLayout(attributeLayout);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
  setLayout(mainLayout);
}

PdfPage::~PdfPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void PdfPage::onPresetChanged() {
  int val = static_cast<int>(m_presetDropdown->getValue());
  m_qualitySlider->blockSignals(true);
  if (val == 1) {
    m_qualitySlider->setValue(20);
  } else if (val == 2) {
    m_qualitySlider->setValue(50);
  } else if (val == 3) {
    m_qualitySlider->setValue(80);
  }
  m_qualitySlider->blockSignals(false);
}

void PdfPage::onQualitySliderChanged() {
  int val = m_qualitySlider->getValue();
  m_presetDropdown->blockSignals(true);
  if (val == 20) {
    m_presetDropdown->setValue(1);
  } else if (val == 50) {
    m_presetDropdown->setValue(2);
  } else if (val == 80) {
    m_presetDropdown->setValue(3);
  } else {
    m_presetDropdown->setValue(0);
  }
  m_presetDropdown->blockSignals(false);
}

void PdfPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  m_progressBar->setVisible(isProcessing);
  m_qualitySlider->setEnabled(!isProcessing);
  m_presetDropdown->setEnabled(!isProcessing);
  m_dragWidget->setEnabled(!isProcessing);
}

void PdfPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Cancelling...");
  }
}

void PdfPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    MessageBoxWidget messageBox("Error", "No PDF file selected!",
                                MessageBoxWidget::Critical, this);
    messageBox.exec();
    return;
  }

  int compressionQuality = m_qualitySlider->getValue();

  if (sourcePaths.size() == 1) {
    compressSinglePdf(sourcePaths.first(), compressionQuality);
  } else {
    compressBatchPdf(sourcePaths, compressionQuality);
  }
}

void PdfPage::compressSinglePdf(const QString &sourcePath, int quality) {
  QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Compressed PDF",
      QFileInfo(sourcePath)
          .dir()
          .filePath(QFileInfo(sourcePath).baseName() + "_compressed.pdf"),
      "PDF Files (*.pdf)");

  if (outputPath.isEmpty()) {
    return;
  }

  setProcessingState(true);
  m_isCancelled = false;
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  m_progressBar->setFormat("Compressing PDF... %p%");

  QFuture<void> future = QtConcurrent::run([this, sourcePath, outputPath, quality]() {
    auto progressCallback = [this](int current, int total) {
      QMetaObject::invokeMethod(
          this,
          [this, current, total]() {
            m_progressBar->setRange(0, total);
            m_progressBar->setValue(current);
            m_progressBar->setFormat(QString("Compressing page %v of %m (%p%)"));
          },
          Qt::QueuedConnection);
    };

    bool success = compressPdf(sourcePath, outputPath, quality, progressCallback);

    QMetaObject::invokeMethod(
        this,
        [this, sourcePath, success, outputPath]() {
          setProcessingState(false);
          m_cancelButton->setText("Cancel");

          if (success) {
            qint64 origSize = QFileInfo(sourcePath).size();
            qint64 compressedSize = QFileInfo(outputPath).size();
            QString sizeStats;
            if (origSize > 0 && compressedSize > 0) {
              double diffPct =
                  (1.0 - (static_cast<double>(compressedSize) / origSize)) * 100.0;
              sizeStats =
                  QString("\n\nOriginal: %1\nCompressed: %2 (%3% %4)")
                      .arg(DropFileWidget::formatFileSize(origSize))
                      .arg(DropFileWidget::formatFileSize(compressedSize))
                      .arg(qAbs(diffPct), 0, 'f', 1)
                      .arg(diffPct >= 0 ? "smaller" : "larger");
            }

            MessageBoxWidget messageBox(
                "Success",
                QString("PDF compressed successfully!\nSaved to: %1%2")
                    .arg(outputPath)
                    .arg(sizeStats),
                MessageBoxWidget::Information, this);
            messageBox.exec();
          } else if (m_isCancelled) {
            MessageBoxWidget messageBox(
                "Cancelled", "PDF compression was cancelled.",
                MessageBoxWidget::Information, this);
            messageBox.exec();
          } else {
            MessageBoxWidget messageBox(
                "Error", "Failed to compress PDF. Please try again.",
                MessageBoxWidget::Critical, this);
            messageBox.exec();
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

void PdfPage::compressBatchPdf(const QStringList &sourcePaths, int quality) {
  QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Compressed PDFs", QDir::homePath());

  if (outputDir.isEmpty()) {
    return;
  }

  setProcessingState(true);
  m_isCancelled = false;
  int totalFiles = sourcePaths.size();
  m_progressBar->setRange(0, totalFiles);
  m_progressBar->setValue(0);
  m_progressBar->setFormat(QString("Compressing %v of %m PDFs (%p%)"));

  QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir, quality, totalFiles]() {
    int successCount = 0;
    int failureCount = 0;
    qint64 totalOrigBytes = 0;
    qint64 totalOutputBytes = 0;
    QSet<QString> usedBaseNames;

    for (int i = 0; i < totalFiles; ++i) {
      if (m_isCancelled) {
        break;
      }

      const QString &sourcePath = sourcePaths.at(i);
      QFileInfo fileInfo(sourcePath);
      QString baseName = fileInfo.baseName() + "_compressed";
      QString uniqueBaseName = baseName;
      int suffix = 1;
      while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
             QFile::exists(QDir(outputDir).filePath(uniqueBaseName + ".pdf"))) {
        uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
      }
      usedBaseNames.insert(uniqueBaseName.toLower());

      QString outputPath =
          QDir(outputDir).filePath(uniqueBaseName + ".pdf");

      if (compressPdf(sourcePath, outputPath, quality, nullptr)) {
        successCount++;
        totalOrigBytes += fileInfo.size();
        totalOutputBytes += QFileInfo(outputPath).size();
      } else {
        failureCount++;
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
        [this, successCount, failureCount, totalOrigBytes, totalOutputBytes]() {
          setProcessingState(false);
          m_cancelButton->setText("Cancel");

          QString title = m_isCancelled ? "Batch Compression Cancelled"
                                        : "Batch Compression Complete";
          QString message = QString("%1 PDF(s) compressed successfully.\n%2 PDF(s) failed.")
                                .arg(successCount)
                                .arg(failureCount);

          if (totalOrigBytes > 0 && totalOutputBytes > 0 && successCount > 0) {
            double diffPct =
                (1.0 - (static_cast<double>(totalOutputBytes) / totalOrigBytes)) * 100.0;
            message += QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
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

bool PdfPage::compressPdf(const QString &inputPath, const QString &outputPath,
                          int quality, const std::function<void(int, int)> &onPageProgress) {
  QPdfDocument pdfDocument;

  // Load the input PDF
  QPdfDocument::Error error = pdfDocument.load(inputPath);
  if (error != QPdfDocument::Error::None) {
    qWarning() << "Failed to load PDF:" << inputPath
               << "Error:" << static_cast<int>(error);
    return false;
  }

  int totalPages = pdfDocument.pageCount();
  if (totalPages <= 0) {
    qWarning() << "PDF has no pages:" << inputPath;
    return false;
  }

  // Use temporary output path to avoid overwriting or corrupting existing destination on failure/cancellation
  QString tempOutputPath = outputPath + ".tmp";

  // Create output PDF writer
  QPdfWriter pdfWriter(tempOutputPath);

  // Configure quality settings
  int dpi = calculateDPI(quality);
  pdfWriter.setResolution(dpi);
  pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

  // Set initial page size based on first page
  QSizeF firstPageSize = pdfDocument.pagePointSize(0);
  pdfWriter.setPageSize(QPageSize(firstPageSize, QPageSize::Point));

  QPainter painter(&pdfWriter);
  if (!painter.isActive()) {
    qWarning() << "Failed to create painter for output PDF";
    QFile::remove(tempOutputPath);
    return false;
  }
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.setRenderHint(QPainter::Antialiasing);

  // Process each page
  for (int i = 0; i < totalPages; ++i) {
    if (m_isCancelled) {
      painter.end();
      QFile::remove(tempOutputPath);
      return false;
    }

    QSizeF pageSize = pdfDocument.pagePointSize(i);
    pdfWriter.setPageSize(QPageSize(pageSize, QPageSize::Point));

    if (i > 0) {
      if (!pdfWriter.newPage()) {
        qWarning() << "Failed to create new page" << i;
        painter.end();
        QFile::remove(tempOutputPath);
        return false;
      }
    }

    qreal scaleFactor = calculateScaleFactor(quality);

    // Calculate render size based on quality
    QSize renderSize(static_cast<int>(pageSize.width() * scaleFactor),
                     static_cast<int>(pageSize.height() * scaleFactor));

    // Render page to image
    QImage pageImage = pdfDocument.render(i, renderSize);

    if (pageImage.isNull()) {
      qWarning() << "Failed to render page" << i;
      painter.end();
      QFile::remove(tempOutputPath);
      return false;
    }

    // Apply compression to the image
    pageImage = compressImage(pageImage, quality);

    // Draw the compressed image to the new PDF
    QRect pageRect =
        pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    painter.drawImage(pageRect, pageImage);

    if (onPageProgress) {
      onPageProgress(i + 1, totalPages);
    }
  }

  painter.end();

  if (m_isCancelled) {
    QFile::remove(tempOutputPath);
    return false;
  }

  // Atomically move temporary file to final output path with backup preservation
  const bool hadExisting = QFile::exists(outputPath);
  const QString backupPath = outputPath + ".bak";
  if (hadExisting) {
    QFile::remove(backupPath);
    if (!QFile::rename(outputPath, backupPath)) {
      QFile::remove(tempOutputPath);
      return false;
    }
  }
  if (!QFile::rename(tempOutputPath, outputPath)) {
    QFile::remove(tempOutputPath);
    if (hadExisting) {
      QFile::rename(backupPath, outputPath);
    }
    return false;
  }
  if (hadExisting) {
    QFile::remove(backupPath);
  }

  return true;
}

QImage PdfPage::compressImage(const QImage &originalImage, int quality) {
  QImage processedImage = originalImage;

  // Reduce color depth for higher compression
  if (quality < 30) {
    // Convert to indexed color for maximum compression
    processedImage = originalImage.convertToFormat(
        QImage::Format_Indexed8, Qt::ColorOnly | Qt::DiffuseDither);
  } else if (quality < 60) {
    // Convert to RGB888 for medium compression
    processedImage = originalImage.convertToFormat(QImage::Format_RGB888);
  }

  // Scale down image for very low quality
  if (quality < 20) {
    QSize newSize = originalImage.size() * 0.6; // Scale to 60%
    processedImage = processedImage.scaled(newSize, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
  } else if (quality < 40) {
    QSize newSize = originalImage.size() * 0.8; // Scale to 80%
    processedImage = processedImage.scaled(newSize, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
  }

  return processedImage;
}

int PdfPage::calculateDPI(int quality) {
  int minDPI = 72;
  int maxDPI = 200;
  return minDPI + ((maxDPI - minDPI) * quality) / 100;
}

qreal PdfPage::calculateScaleFactor(int quality) {
  qreal minScale = 0.5;
  qreal maxScale = 2.0;
  return minScale + ((maxScale - minScale) * quality) / 100.0;
}
