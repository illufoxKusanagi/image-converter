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
  QPdfDocument pdfDocument;

  // Load the input PDF
  QPdfDocument::Error error = pdfDocument.load(inputPath);
  if (error != QPdfDocument::Error::None) {
    qWarning() << "Failed to load PDF:" << inputPath
               << "Error:" << static_cast<int>(error);
    return false;
  }

  // Create output PDF writer
  QPdfWriter pdfWriter(outputPath);

  // Configure quality settings
  int dpi = calculateDPI(quality);
  pdfWriter.setResolution(dpi);

  // Set page size based on first page if available
  if (pdfDocument.pageCount() > 0) {
    QSizeF pageSize = pdfDocument.pagePointSize(0);
    pdfWriter.setPageSize(QPageSize(pageSize, QPageSize::Point));
  }

  QPainter painter(&pdfWriter);
  if (!painter.isActive()) {
    qWarning() << "Failed to create painter for output PDF";
    return false;
  }

  // Process each page
  for (int i = 0; i < pdfDocument.pageCount(); ++i) {
    if (i > 0) {
      if (!pdfWriter.newPage()) {
        qWarning() << "Failed to create new page" << i;
        continue;
      }
    }

    // Get page size for this specific page
    QSizeF pageSize = pdfDocument.pagePointSize(i);
    qreal scaleFactor = calculateScaleFactor(quality);

    // Calculate render size based on quality
    QSize renderSize(static_cast<int>(pageSize.width() * scaleFactor),
                     static_cast<int>(pageSize.height() * scaleFactor));

    // Render page to image
    QImage pageImage = pdfDocument.render(i, renderSize);

    if (pageImage.isNull()) {
      qWarning() << "Failed to render page" << i;
      continue;
    }

    // Apply compression to the image
    pageImage = compressImage(pageImage, quality);

    // Draw the compressed image to the new PDF
    QRect pageRect =
        pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    painter.drawImage(pageRect, pageImage);
  }

  painter.end();
  return true;
}

// bool PdfPage::compressPdf(const QString &inputPath, const QString
// &outputPath,
//                           int quality) {
//   QPdfDocument pdfDocument;

//   if (pdfDocument.load(inputPath) != QPdfDocument::Error::None) {
//     return false;
//   }

//   QPdfWriter pdfWriter(outputPath);

//   // More conservative DPI range (72-150 instead of 72-200)
//   int dpi = 72 + (quality * 78) / 100; // 72-150 DPI
//   pdfWriter.setResolution(dpi);

//   // Set initial page size before creating painter
//   if (pdfDocument.pageCount() > 0) {
//     QSizeF pageSize = pdfDocument.pagePointSize(0);
//     pdfWriter.setPageSize(QPageSize(pageSize, QPageSize::Point));
//   }

//   QPainter painter(&pdfWriter);
//   if (!painter.isActive())
//     return false;

//   for (int i = 0; i < pdfDocument.pageCount(); ++i) {
//     if (i > 0) {
//       if (!pdfWriter.newPage()) {
//         qWarning() << "Failed to create new page" << i;
//         continue;
//       }
//     }

//     // Get page size
//     QSizeF pageSize = pdfDocument.pagePointSize(i);

//     // Conservative scale factor (0.5-1.0)
//     qreal scale = 0.5 + (quality * 0.5) / 100.0;
//     QSize renderSize(static_cast<int>(pageSize.width() * scale),
//                      static_cast<int>(pageSize.height() * scale));

//     // Ensure minimum render size
//     renderSize.setWidth(qMax(renderSize.width(), 150));
//     renderSize.setHeight(qMax(renderSize.height(), 150));

//     QImage pageImage = pdfDocument.render(i, renderSize);
//     if (pageImage.isNull()) {
//       qWarning() << "Failed to render page" << i;
//       continue;
//     }

//     // Apply compression
//     pageImage = smartCompress(pageImage, quality);

//     // Draw image to fill the page
//     QRect pageRect =
//         pdfWriter.pageLayout().fullRectPixels(pdfWriter.resolution());
//     painter.drawImage(pageRect, pageImage);
//   }

//   painter.end();
//   return true;
// }

QImage PdfPage::compressImage(const QImage &originalImage, int quality) {
  QImage processedImage = originalImage; // Start with a copy

  // Step 1: Format Conversion (Crucially, preserve Alpha if present)
  if (originalImage.hasAlphaChannel()) {
    // If the original image has an alpha channel, ensure the processed image
    // is in a format that supports alpha. ARGB32_Premultiplied is a good
    // default as it's well-supported and handles alpha correctly.
    if (processedImage.format() != QImage::Format_ARGB32_Premultiplied &&
        processedImage.format() != QImage::Format_ARGB32) {
      processedImage =
          processedImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
  } else {
    // If the original image does not have an alpha channel,
    // we can convert to an opaque format like RGB888. This might offer
    // slightly better compression for purely opaque images by the PDF writer,
    // or if the original format was less common.
    if (processedImage.format() != QImage::Format_RGB888 &&
        processedImage.format() !=
            QImage::Format_RGB32) { // Check against common opaque formats
      processedImage = processedImage.convertToFormat(QImage::Format_RGB888);
    }
  }

  // Step 2: Scaling (applied after format decision)
  // This logic primarily affects image dimensions and thus file size.
  // It does not directly interfere with the alpha channel if the format
  // supports it.
  if (quality < 20) { // Very high compression, significantly reduce size
    QSize newSize = processedImage.size() * 0.6; // Scale to 60%
    processedImage = processedImage.scaled(newSize, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
  } else if (quality < 40) { // High compression, moderately reduce size
    QSize newSize = processedImage.size() * 0.8; // Scale to 80%
    processedImage = processedImage.scaled(newSize, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
  }
  // For quality >= 40, this specific scaling logic doesn't apply.
  // The PDF writer will still apply its own compression algorithms (like JPEG
  // or Flate) to the image data based on the image type and content.

  return processedImage;
}

QImage PdfPage::smartCompress(const QImage &image, int quality) {
  // Ensure the image is in a valid format
  QImage workingImage = image;
  if (workingImage.isNull() || workingImage.width() == 0 ||
      workingImage.height() == 0) {
    qWarning() << "Invalid image for compression";
    return image;
  }

  // Convert to RGB for better JPEG compression
  if (workingImage.format() != QImage::Format_RGB888) {
    workingImage = workingImage.convertToFormat(QImage::Format_RGB888);
  }

  // Use JPEG compression for quality < 80
  if (quality < 80) {
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);

    int jpegQuality = 30 + (quality * 65 / 100); // 30-95 JPEG quality

    if (workingImage.save(&buffer, "JPEG", jpegQuality)) {
      QImage compressed;
      if (compressed.loadFromData(data, "JPEG")) {
        return compressed;
      }
    }

    // If JPEG compression fails, return the RGB converted image
    qWarning() << "JPEG compression failed, returning RGB image";
  }

  return workingImage;
}

int PdfPage::calculateDPI(int quality) {
  // Map quality (0-100) to DPI (72-200)
  // Lower quality = lower DPI = smaller file
  int minDPI = 72;
  int maxDPI = 400;
  return minDPI + ((maxDPI - minDPI) * quality) / 100;
}

qreal PdfPage::calculateScaleFactor(int quality) {
  // Map quality to scale factor for rendering (0.5-2.0)
  qreal minScale = 0.5;
  qreal maxScale = 2.0;
  return minScale + ((maxScale - minScale) * quality) / 100.0;
}
