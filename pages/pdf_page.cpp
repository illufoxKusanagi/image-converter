#include "pdf_page.h"

PdfPage::PdfPage(QWidget *parent) : QWidget(parent) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignHCenter);
  m_dragWidget = new DropFileWidget(this, "PDF");
  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  ButtonAction *processButton = new ButtonAction(this, "Compress PDF", "no");
  processButton->setEnabled(true);
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  mainLayout->addWidget(m_dragWidget);
  mainLayout->addWidget(m_qualitySlider);
  mainLayout->addWidget(processButton);
  connect(processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);
  setLayout(mainLayout);
}

void PdfPage::onProcessButtonClicked() {
  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    MessageBoxWidget messageBox("Error", "No PDF file selected!",
                                MessageBoxWidget::Critical);
    messageBox.exec();
    return;
  }

  int compressionQuality = m_qualitySlider->getValue(); // 0-100 scale
  qDebug() << "Selected compression quality:" << compressionQuality;

  if (sourcePaths.size() == 1) {
    // Single file compression
    compressSinglePdf(sourcePaths.first(), compressionQuality);
  } else {
    // Batch compression
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

  if (outputPath.isEmpty())
    return;

  if (compressPdf(sourcePath, outputPath, quality)) {
    MessageBoxWidget messageBox("Success", "PDF compressed successfully!",
                                MessageBoxWidget::Information);
    messageBox.exec();
  } else {
    MessageBoxWidget messageBox("Error", "Failed to compress PDF!",
                                MessageBoxWidget::Critical);
    messageBox.exec();
  }
}
void PdfPage::compressBatchPdf(const QStringList &sourcePaths, int quality) {
  QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Compressed PDFs", QDir::homePath());

  if (outputDir.isEmpty())
    return;

  int successCount = 0;
  int failureCount = 0;

  for (const QString &sourcePath : sourcePaths) {
    QFileInfo fileInfo(sourcePath);
    QString outputPath =
        QDir(outputDir).filePath(fileInfo.baseName() + "_compressed.pdf");

    if (compressPdf(sourcePath, outputPath, quality)) {
      successCount++;
    } else {
      failureCount++;
    }
  }

  MessageBoxWidget messageBox(
      "Batch Compression Complete",
      QString("%1 PDF(s) compressed successfully.\n%2 PDF(s) failed.")
          .arg(successCount)
          .arg(failureCount),
      MessageBoxWidget::Information);
  messageBox.exec();
}

bool PdfPage::compressPdf(const QString &inputPath, const QString &outputPath,
                          int quality) {
  QPdfDocument sourceDoc;
  if (sourceDoc.load(inputPath) != QPdfDocument::Error::None) {
    return false;
  }

  QPdfWriter writer(outputPath);
  writer.setPdfVersion(QPagedPaintDevice::PdfVersion_1_4);

  // DON'T start the painter yet - set page properties first
  for (int i = 0; i < sourceDoc.pageCount(); ++i) {

    // Get the EXACT original page size
    QSizeF originalPageSize = sourceDoc.pagePointSize(i);

    if (i == 0) {
      // Set up the writer properties BEFORE creating painter
      writer.setPageSize(QPageSize(originalPageSize, QPageSize::Point));

      // Set layout with NO margins
      QPageLayout layout;
      layout.setPageSize(QPageSize(originalPageSize, QPageSize::Point));
      layout.setMargins(QMarginsF(0, 0, 0, 0));
      layout.setOrientation(QPageLayout::Portrait);
      writer.setPageLayout(layout);

      // Set resolution
      int dpi = 96 + (quality * 54 / 100);
      writer.setResolution(dpi);
    }
  }

  // NOW create the painter after all settings are done
  QPainter painter(&writer);
  if (!painter.isActive())
    return false;

  // FIXED: Disable ALL antialiasing to prevent grey borders
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
  painter.setRenderHint(QPainter::TextAntialiasing,
                        false); // Even disable text AA
  painter.setRenderHint(QPainter::VerticalSubpixelPositioning, false);

  for (int i = 0; i < sourceDoc.pageCount(); ++i) {
    if (i > 0) {
      writer.newPage();
    }

    QSizeF originalPageSize = sourceDoc.pagePointSize(i);

    // Simple render calculation
    int dpi = 96 + (quality * 54 / 100);
    qreal scale = static_cast<qreal>(dpi) / 72.0;

    QSize renderSize(static_cast<int>(originalPageSize.width() * scale),
                     static_cast<int>(originalPageSize.height() * scale));

    // FIXED: Render with specific options to avoid borders
    QImage pageImage = sourceDoc.render(i, renderSize);
    if (pageImage.isNull())
      continue;

    // FIXED: Crop any potential border pixels
    if (pageImage.width() > 10 && pageImage.height() > 10) {
      // Crop 1-2 pixels from each edge to remove border artifacts
      QRect cropRect(1, 1, pageImage.width() - 2, pageImage.height() - 2);
      pageImage = pageImage.copy(cropRect);
    }

    if (quality < 60) {
      pageImage = simpleCompress(pageImage, quality);
    }

    // FIXED: Use exact pixel-perfect positioning
    QRect paintRect = painter.viewport();

    // Ensure the image fills exactly without sub-pixel positioning
    painter.drawImage(paintRect, pageImage, pageImage.rect());

    qDebug() << "Page" << (i + 1) << "painted to viewport:" << paintRect;
  }

  painter.end();
  return true;
}

// Simple compression without format conversion madness
QImage PdfPage::simpleCompress(const QImage &image, int quality) {
  if (quality >= 60)
    return image; // No compression needed

  // Simple JPEG compression
  QByteArray data;
  QBuffer buffer(&data);
  buffer.open(QIODevice::WriteOnly);

  // Map 0-60 quality to 50-85 JPEG quality
  int jpegQuality = 50 + (quality * 35 / 60);

  // FIXED: Convert to RGB format that eliminates border artifacts
  QImage cleanImage = image.convertToFormat(QImage::Format_RGB888);

  // FIXED: Fill any transparent/grey pixels with white
  if (cleanImage.hasAlphaChannel()) {
    for (int y = 0; y < cleanImage.height(); ++y) {
      for (int x = 0; x < cleanImage.width(); ++x) {
        QColor pixel = cleanImage.pixelColor(x, y);
        if (pixel.alpha() < 255) {
          cleanImage.setPixelColor(x, y, Qt::white);
        }
      }
    }
  }

  if (cleanImage.save(&buffer, "JPEG", jpegQuality)) {
    QImage compressed;
    if (compressed.loadFromData(data, "JPEG")) {
      return compressed;
    }
  }

  return image; // Fallback to original
}

int PdfPage::calculateOptimalDPI(int quality) {
  // More conservative DPI range to prevent memory issues
  if (quality >= 90)
    return 300; // High quality
  if (quality >= 70)
    return 200; // Medium-high quality
  if (quality >= 50)
    return 150; // Medium quality
  if (quality >= 30)
    return 120; // Lower quality
  return 96;    // Minimum quality
}

QImage PdfPage::optimizeImageCompression(const QImage &originalImage,
                                         int quality) {
  if (originalImage.isNull()) {
    qWarning() << "Null image passed to compression";
    return originalImage;
  }

  QImage workingImage = originalImage;

  // Convert to optimal format based on content
  if (workingImage.hasAlphaChannel()) {
    // Preserve alpha channel for transparent content
    workingImage =
        workingImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  } else {
    // Use RGB for opaque content
    workingImage = workingImage.convertToFormat(QImage::Format_RGB888);
  }

  // Apply JPEG compression for lower quality settings
  if (quality < 80) {
    QByteArray compressedData;
    QBuffer buffer(&compressedData);
    buffer.open(QIODevice::WriteOnly);

    // Map quality to JPEG quality (40-90 range)
    int jpegQuality = qMax(40, qMin(90, 40 + (quality * 50 / 100)));

    if (workingImage.save(&buffer, "JPEG", jpegQuality)) {
      QImage compressedImage;
      if (compressedImage.loadFromData(compressedData, "JPEG")) {
        qDebug() << "Applied JPEG compression with quality:" << jpegQuality;
        return compressedImage;
      }
    }

    qWarning() << "JPEG compression failed, using original";
  }

  return workingImage;
}

QImage PdfPage::applySmartCompression(const QImage &image, int quality) {
  QImage result = image.convertToFormat(QImage::Format_RGB888);

  // Apply JPEG compression for file size reduction
  QByteArray compressedData;
  QBuffer buffer(&compressedData);
  buffer.open(QIODevice::WriteOnly);

  // Map quality slider to JPEG quality (30-85 range for good compression)
  int jpegQuality = 30 + (quality * 55 / 100);

  if (result.save(&buffer, "JPEG", jpegQuality)) {
    QImage compressedImage;
    if (compressedImage.loadFromData(compressedData, "JPEG")) {
      qDebug() << "Applied JPEG compression - Quality:" << jpegQuality
               << "Original size:" << image.size()
               << "Compressed size:" << compressedImage.size();
      return compressedImage;
    }
  }

  qWarning() << "Compression failed, using original";
  return result;
}

qreal PdfPage::calculateOptimalScale(int quality, const QSizeF &pageSize) {
  // Base scale factor based on quality (0.6 to 1.2 range)
  qreal baseScale = 0.6 + (quality * 0.6 / 100.0);

  // Adjust based on page dimensions
  qreal maxDimension = qMax(pageSize.width(), pageSize.height());
  qreal adjustmentFactor = 1.0;

  if (maxDimension > 1200) {
    adjustmentFactor = 0.8; // Reduce scale for very large pages
  } else if (maxDimension < 600) {
    adjustmentFactor = 1.2; // Increase scale for small pages
  }

  qreal finalScale = baseScale * adjustmentFactor;

  // Clamp to reasonable bounds
  return qBound(0.4, finalScale, 1.5);
}

// FIXED: New minimal compression method that preserves text
QImage PdfPage::applyMinimalCompression(const QImage &image, int quality) {
  // Don't convert format - this often causes black pages
  QImage result = image;

  // Only apply compression for very low quality settings
  if (quality < 30) {
    // Very conservative JPEG compression
    QByteArray compressedData;
    QBuffer buffer(&compressedData);
    buffer.open(QIODevice::WriteOnly);

    // Use higher JPEG quality to preserve text readability
    int jpegQuality = qMax(60, 60 + (quality * 30 / 30)); // 60-90 range

    // Convert to RGB only if necessary
    QImage tempImage = result;
    if (tempImage.format() != QImage::Format_RGB888) {
      tempImage = tempImage.convertToFormat(QImage::Format_RGB888);
    }

    if (tempImage.save(&buffer, "JPEG", jpegQuality)) {
      QImage compressedImage;
      if (compressedImage.loadFromData(compressedData, "JPEG")) {
        qDebug() << "Applied minimal JPEG compression - Quality:"
                 << jpegQuality;
        return compressedImage;
      }
    }
  }

  return result; // Return original if compression fails or not needed
}
