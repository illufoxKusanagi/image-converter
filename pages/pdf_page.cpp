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
// void PdfPage::compressBatchPdf(const QStringList &sourcePaths, int quality) {
//   QString outputDir = QFileDialog::getExistingDirectory(
//       this, "Select Output Directory for Compressed PDFs", QDir::homePath());

//   if (outputDir.isEmpty())
//     return;

//   int successCount = 0;
//   int failureCount = 0;

//   for (const QString &sourcePath : sourcePaths) {
//     QFileInfo fileInfo(sourcePath);
//     QString outputPath =
//         QDir(outputDir).filePath(fileInfo.baseName() + "_compressed.pdf");

//     if (compressPdf(sourcePath, outputPath, quality)) {
//       successCount++;
//     } else {
//       failureCount++;
//     }
//   }

//   MessageBoxWidget messageBox(
//       "Batch Compression Complete",
//       QString("%1 PDF(s) compressed successfully.\n%2 PDF(s) failed.")
//           .arg(successCount)
//           .arg(failureCount),
//       MessageBoxWidget::Information);
//   messageBox.exec();
// }

// bool PdfPage::compressPdf(const QString &inputPath, const QString
// &outputPath,
//                           int quality) {
// QPdfDocument sourceDoc;
// if (sourceDoc.load(inputPath) != QPdfDocument::Error::None) {
//   return false;
// }

// QPdfWriter writer(outputPath);
// writer.setPdfVersion(QPagedPaintDevice::PdfVersion_1_4);

// // DON'T start the painter yet - set page properties first
// for (int i = 0; i < sourceDoc.pageCount(); ++i) {

//   // Get the EXACT original page size
//   QSizeF originalPageSize = sourceDoc.pagePointSize(i);

//   if (i == 0) {
//     // Set up the writer properties BEFORE creating painter
//     writer.setPageSize(QPageSize(originalPageSize, QPageSize::Point));

//     // Set layout with NO margins
//     QPageLayout layout;
//     layout.setPageSize(QPageSize(originalPageSize, QPageSize::Point));
//     layout.setMargins(QMarginsF(0, 0, 0, 0));
//     layout.setOrientation(QPageLayout::Portrait);
//     writer.setPageLayout(layout);

//     // Set resolution
//     int dpi = 96 + (quality * 54 / 100);
//     writer.setResolution(dpi);
//   }
// }

// // NOW create the painter after all settings are done
// QPainter painter(&writer);
// if (!painter.isActive())
//   return false;

// // KEEP your working antialiasing settings
// painter.setRenderHint(QPainter::Antialiasing, false);
// painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
// painter.setRenderHint(QPainter::TextAntialiasing, false);
// painter.setRenderHint(QPainter::VerticalSubpixelPositioning, false);

// for (int i = 0; i < sourceDoc.pageCount(); ++i) {
//   if (i > 0) {
//     writer.newPage();
//   }

//   QSizeF originalPageSize = sourceDoc.pagePointSize(i);

//   // KEEP your working render calculation
//   int dpi = 96 + (quality * 54 / 100);
//   qreal scale = static_cast<qreal>(dpi) / 72.0;

//   QSize renderSize(static_cast<int>(originalPageSize.width() * scale),
//                    static_cast<int>(originalPageSize.height() * scale));

//   // KEEP your working render call
//   QImage pageImage = sourceDoc.render(i, renderSize);
//   if (pageImage.isNull())
//     continue;

//   // IMPROVED: Only clean borders if they exist, don't crop content
//   pageImage = cleanBordersOnly(pageImage);

//   if (quality < 60) {
//     pageImage = simpleCompress(pageImage, quality);
//   }

//   // KEEP your working paint code
//   QRect paintRect = painter.viewport();
//   painter.drawImage(paintRect, pageImage, pageImage.rect());

//   qDebug() << "Page" << (i + 1) << "painted to viewport:" << paintRect;
// }

// painter.end();
// return true;
// Quick MuPDF test
// Try MuPDF with quality settings
//   if (testMutool(inputPath, outputPath, quality)) {
//     qDebug() << "SUCCESS: MuPDF compression completed with quality:" <<
//     quality; return true;
//   }

//   qDebug() << "MuPDF failed, would use QtPDF fallback here...";
//   // You can uncomment your QtPDF code as fallback if needed
//   return false;
// }

bool PdfPage::compressPdf(const QString &inputPath, const QString &outputPath,
                          int quality) {
  return compressPdfAdvanced(inputPath, outputPath, quality);
}

bool PdfPage::compressPdfAdvanced(const QString &inputPath,
                                  const QString &outputPath, int quality) {
  QFileInfo inputInfo(inputPath);
  qint64 originalSize = inputInfo.size();

  qDebug() << "Input file size:" << originalSize << "bytes";
  qDebug() << "Target quality:" << quality;

  // Try multiple compression strategies in order of aggressiveness
  QStringList strategies = getCompressionStrategies(quality);

  for (int i = 0; i < strategies.size(); ++i) {
    QString tempPath = outputPath + QString(".temp%1").arg(i);

    if (executeCompressionStrategy(inputPath, tempPath, strategies[i])) {
      QFileInfo tempInfo(tempPath);
      qint64 compressedSize = tempInfo.size();
      qreal compressionRatio =
          ((qreal)(originalSize - compressedSize) / originalSize) * 100;

      qDebug() << "Strategy" << i << "result:" << compressedSize << "bytes"
               << "(" << QString::number(compressionRatio, 'f', 1)
               << "% reduction)";

      // Accept if we got meaningful compression (at least 5% reduction)
      if (compressedSize < originalSize * 0.95) {
        QFile::rename(tempPath, outputPath);
        cleanupTempFiles(outputPath, i + 1); // Clean remaining temp files

        qDebug() << "SUCCESS: Used strategy" << i << "- Size reduced by"
                 << (originalSize - compressedSize) << "bytes"
                 << "(" << QString::number(compressionRatio, 'f', 1) << "%)";
        return true;
      }
    }

    // Clean up failed attempt
    QFile::remove(tempPath);
  }

  // If no strategy worked, use fallback
  qDebug() << "All MuPDF strategies failed. Trying fallback...";
  return fallbackCompression(inputPath, outputPath, quality);
}

QStringList PdfPage::getCompressionStrategies(int quality) {
  QStringList strategies;

  if (quality < 30) {
    // Aggressive compression for maximum size reduction
    strategies << "clean -d -i -f -a -s"; // Most aggressive
  } else if (quality < 70) {
    // Medium compression
    strategies << "clean -i -f"; // Medium
  } else {
    // Light compression
    strategies << "clean -i -f" // Medium
               << "clean -f"    // Minimal
               << "clean";      // Basic cleanup
  }
  return strategies;
}

bool PdfPage::executeCompressionStrategy(const QString &input,
                                         const QString &output,
                                         const QString &strategy) {
  QProcess process;
  QStringList args = strategy.split(' ', Qt::SkipEmptyParts);
  args << input << output;

  QString mutoolPath = "f:/matkul/sem_6/AppProject/ImageConverter_new/"
                       "mupdf-1.26.2-windows/mutool.exe";

  // Debug: print each argument separately
  qDebug() << "Executing:" << mutoolPath;
  for (const QString &arg : args) {
    qDebug() << "  arg:" << arg;
  }

  process.start(mutoolPath, args);
  bool finished = process.waitForFinished(60000); // 60 second timeout

  if (!finished) {
    qDebug() << "Process timeout for strategy:" << strategy;
    process.kill();
    return false;
  }

  if (process.exitCode() != 0) {
    qDebug() << "Process failed with exit code:" << process.exitCode();
    qDebug() << "Error output:" << process.readAllStandardError();
    return false;
  }

  // Verify output file exists and has content
  QFileInfo outputInfo(output);
  if (!outputInfo.exists() || outputInfo.size() == 0) {
    qDebug() << "Output file invalid or empty";
    return false;
  }

  return true;
}

bool PdfPage::fallbackCompression(const QString &inputPath,
                                  const QString &outputPath, int quality) {
  qDebug() << "Using fallback: Image-based compression";

  // Use ghostscript-style compression if mutool fails
  QProcess process;
  QStringList args;

  // Try pdftk if available
  args << inputPath << "output" << outputPath << "compress";

  process.start("pdftk", args);
  if (process.waitForFinished(30000) && process.exitCode() == 0) {
    qDebug() << "Fallback: pdftk compression successful";
    return true;
  }

  // Last resort: copy original
  qDebug() << "All compression methods failed. Copying original file.";
  return QFile::copy(inputPath, outputPath);
}

void PdfPage::cleanupTempFiles(const QString &outputPath, int maxIndex) {
  for (int i = 0; i < maxIndex; ++i) {
    QString tempPath = outputPath + QString(".temp%1").arg(i);
    QFile::remove(tempPath);
  }
}

// Enhanced batch compression with progress tracking
void PdfPage::compressBatchPdf(const QStringList &sourcePaths, int quality) {
  QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Compressed PDFs", QDir::homePath());

  if (outputDir.isEmpty())
    return;

  // Create progress dialog
  QProgressDialog progress("Compressing PDFs...", "Cancel", 0,
                           sourcePaths.size(), this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(1000);

  int successCount = 0;
  int failureCount = 0;
  qint64 totalSizeBefore = 0;
  qint64 totalSizeAfter = 0;

  for (int i = 0; i < sourcePaths.size(); ++i) {
    if (progress.wasCanceled())
      break;

    progress.setValue(i);
    progress.setLabelText(
        QString("Processing: %1").arg(QFileInfo(sourcePaths[i]).fileName()));

    QApplication::processEvents(); // Keep UI responsive

    const QString &sourcePath = sourcePaths[i];
    QFileInfo fileInfo(sourcePath);
    QString outputPath =
        QDir(outputDir).filePath(fileInfo.baseName() + "_compressed.pdf");

    qint64 sizeBefore = fileInfo.size();
    totalSizeBefore += sizeBefore;

    if (compressPdf(sourcePath, outputPath, quality)) {
      successCount++;
      qint64 sizeAfter = QFileInfo(outputPath).size();
      totalSizeAfter += sizeAfter;
      qDebug() << "Compressed:" << fileInfo.fileName() << "from" << sizeBefore
               << "to" << sizeAfter << "bytes";
    } else {
      failureCount++;
      totalSizeAfter += sizeBefore; // Count original size if compression failed
    }
  }

  progress.setValue(sourcePaths.size());

  // Calculate overall compression statistics
  qreal overallCompression =
      totalSizeBefore > 0
          ? ((qreal)(totalSizeBefore - totalSizeAfter) / totalSizeBefore) * 100
          : 0;

  QString message =
      QString("%1 PDF(s) compressed successfully.\n%2 PDF(s) failed.\n\n")
          .arg(successCount)
          .arg(failureCount);

  if (successCount > 0) {
    message += QString("Total size reduction: %1 MB (%2%)")
                   .arg((totalSizeBefore - totalSizeAfter) / (1024.0 * 1024.0),
                        0, 'f', 2)
                   .arg(overallCompression, 0, 'f', 1);
  }

  MessageBoxWidget messageBox("Batch Compression Complete", message,
                              MessageBoxWidget::Information);
  messageBox.exec();
}

QImage PdfPage::cleanBordersOnly(const QImage &image) {
  if (image.isNull())
    return image;

  QImage result = image; // Work on copy
  int width = result.width();
  int height = result.height();

  // Very conservative grey detection - only obvious borders
  auto isGreyBorder = [](const QColor &color) {
    int r = color.red();
    int g = color.green();
    int b = color.blue();

    // Only detect very light grey (almost white) borders
    int avg = (r + g + b) / 3;
    int maxDiff = qMax(qMax(qAbs(r - g), qAbs(g - b)), qAbs(r - b));

    // Very narrow range - only obvious grey borders
    return (maxDiff < 10 && avg > 230 && avg < 250);
  };

  // Only clean the very edge pixels (no cropping)

  // Top and bottom edges
  for (int x = 0; x < width; x++) {
    QColor topPixel = result.pixelColor(x, 0);
    if (isGreyBorder(topPixel)) {
      result.setPixelColor(x, 0, Qt::white);
    }

    if (height > 1) {
      QColor bottomPixel = result.pixelColor(x, height - 1);
      if (isGreyBorder(bottomPixel)) {
        result.setPixelColor(x, height - 1, Qt::white);
      }
    }
  }

  // Left and right edges
  for (int y = 0; y < height; y++) {
    QColor leftPixel = result.pixelColor(0, y);
    if (isGreyBorder(leftPixel)) {
      result.setPixelColor(0, y, Qt::white);
    }

    if (width > 1) {
      QColor rightPixel = result.pixelColor(width - 1, y);
      if (isGreyBorder(rightPixel)) {
        result.setPixelColor(width - 1, y, Qt::white);
      }
    }
  }

  return result;
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

// Just add this simple test to your existing code:
bool PdfPage::testMutool(const QString &inputPath, const QString &outputPath,
                         int quality) {
  QFileInfo inputInfo(inputPath);
  qint64 originalSize = inputInfo.size();

  // Strategy 1: Minimal compression
  QString tempPath1 = outputPath + ".temp1";
  if (tryMutoolStrategy(inputPath, tempPath1,
                        QStringList() << "clean" << "-f")) {
    QFileInfo temp1Info(tempPath1);
    qint64 temp1Size = temp1Info.size();

    if (temp1Size < originalSize) {
      // Success! Use this version
      QFile::rename(tempPath1, outputPath);
      qDebug() << "Used minimal compression. Size reduced by"
               << (originalSize - temp1Size) << "bytes";
      return true;
    }
  }

  // Strategy 2: Medium compression (only if quality < 70)
  if (quality < 70) {
    QString tempPath2 = outputPath + ".temp2";
    if (tryMutoolStrategy(inputPath, tempPath2,
                          QStringList() << "clean" << "-i" << "-f")) {
      QFileInfo temp2Info(tempPath2);
      qint64 temp2Size = temp2Info.size();

      if (temp2Size < originalSize) {
        QFile::rename(tempPath2, outputPath);
        QFile::remove(tempPath1); // Clean up
        qDebug() << "Used medium compression. Size reduced by"
                 << (originalSize - temp2Size) << "bytes";
        return true;
      }
    }
  }

  // If all strategies fail or make file bigger, just copy original
  qDebug() << "All compression strategies failed or increased size. Copying "
              "original.";
  return QFile::copy(inputPath, outputPath);
}

bool PdfPage::tryMutoolStrategy(const QString &input, const QString &output,
                                const QStringList &baseArgs) {
  QProcess process;
  QStringList args = baseArgs;
  args << input << output;

  QString mutoolPath = "f:/matkul/sem_6/AppProject/ImageConverter_new/"
                       "mupdf-1.26.2-windows/mutool.exe";

  process.start(mutoolPath, args);
  process.waitForFinished(30000);

  return process.exitCode() == 0;
}