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
  // Map quality (0-100) to DPI (72-200)
  // Lower quality = lower DPI = smaller file
  int minDPI = 72;
  int maxDPI = 200;
  return minDPI + ((maxDPI - minDPI) * quality) / 100;
}

qreal PdfPage::calculateScaleFactor(int quality) {
  // Map quality to scale factor for rendering (0.5-2.0)
  qreal minScale = 0.5;
  qreal maxScale = 2.0;
  return minScale + ((maxScale - minScale) * quality) / 100.0;
}

double PdfPage::calculateRenderDPI(int quality) {
  // This method is kept for compatibility but not used in Qt PDF approach
  double minRenderDPI = 96.0;
  double maxRenderDPI = 300.0;
  return minRenderDPI + ((maxRenderDPI - minRenderDPI) * quality) / 100.0;
}
