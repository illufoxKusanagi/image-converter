#include "pdf_page.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

namespace {
bool replaceDestinationAtomically(const QString &tempOutputPath,
                                  const QString &outputPath) {
  const bool hadExisting = QFile::exists(outputPath);
  const QString backupPath =
      QString("%1.bak.%2").arg(outputPath).arg(QDateTime::currentMSecsSinceEpoch());
  if (hadExisting) {
    if (!QFile::rename(outputPath, backupPath)) {
      QFile::remove(tempOutputPath);
      return false;
    }
  }
  if (!QFile::rename(tempOutputPath, outputPath)) {
    QFile::remove(tempOutputPath);
    if (hadExisting) {
      if (!QFile::rename(backupPath, outputPath)) {
        qWarning() << "Failed to restore backup PDF file from:" << backupPath;
      }
    }
    return false;
  }
  if (hadExisting) {
    QFile::remove(backupPath);
  }
  return true;
}
} // namespace

PdfPage::PdfPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(14);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  m_dragWidget = new DropFileWidget(this, "PDF");
  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setValue(50);

  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  QStringList presetOptions = {
      "Custom", "Web / Email (70-80% smaller)", "Balanced (40-50% smaller)",
      "High Quality (15-25% smaller)"};
  m_presetSelect = new ui::Select(presetOptions, this);
  m_presetSelect->setCurrentIndex(2); // Default to Balanced (50)
  m_presetField = new ui::FormField("Compression Preset", m_presetSelect, this);

  connect(m_presetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfPage::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfPage::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_presetField, 1, Qt::AlignTop);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setFixedHeight(22);
  m_progressBar->setAlignment(Qt::AlignCenter);
  m_progressBar->setTextVisible(true);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Compress PDF", ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);

  m_cancelButton = new ui::Button("Cancel Compression",
                                  ui::ButtonVariant::Destructive,
                                  ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &PdfPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  mainLayout->addLayout(attributeLayout);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &PdfPage::applyThemeStyles);
  applyThemeStyles();
}

PdfPage::~PdfPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void PdfPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  m_progressBar->setFont(t.font(t.sizeXs, QFont::DemiBold));
  m_progressBar->setStyleSheet(
      QString("QProgressBar {"
              "  border: 1px solid %1;"
              "  border-radius: %2px;"
              "  text-align: center;"
              "  background-color: %3;"
              "  color: %4;"
              "}"
              "QProgressBar::chunk {"
              "  background-color: %5;"
              "  border-radius: %6px;"
              "}")
          .arg(ui::StyleHelper::toHexString(c.border))
          .arg(r.md)
          .arg(ui::StyleHelper::toHexString(c.muted))
          .arg(ui::StyleHelper::toHexString(c.foreground))
          .arg(ui::StyleHelper::toHexString(c.primary))
          .arg(r.md > 2 ? r.md - 1 : 2));

  if (m_processButton) {
    m_processButton->setIcon(ui::Icon::get(":/icons/icons/lucide-file-text.svg",
                                           c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(
        ":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void PdfPage::onPresetChanged(int index) {
  if (!m_qualitySlider) return;
  m_qualitySlider->blockSignals(true);
  if (index == 1) {
    m_qualitySlider->setValue(20);
  } else if (index == 2) {
    m_qualitySlider->setValue(50);
  } else if (index == 3) {
    m_qualitySlider->setValue(80);
  }
  m_qualitySlider->blockSignals(false);
}

void PdfPage::onQualitySliderChanged() {
  if (!m_presetSelect || !m_qualitySlider) return;
  int val = m_qualitySlider->getValue();
  m_presetSelect->blockSignals(true);
  if (val == 20) {
    m_presetSelect->setCurrentIndex(1);
  } else if (val == 50) {
    m_presetSelect->setCurrentIndex(2);
  } else if (val == 80) {
    m_presetSelect->setCurrentIndex(3);
  } else {
    m_presetSelect->setCurrentIndex(0);
  }
  m_presetSelect->blockSignals(false);
}

void PdfPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  m_progressBar->setVisible(isProcessing);
  m_qualitySlider->setEnabled(!isProcessing);
  if (m_presetSelect) {
    m_presetSelect->setEnabled(!isProcessing);
  }
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
    MessageBoxWidget messageBox("Error", "Please select at least one PDF file.",
                                MessageBoxWidget::Warning, this);
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

  QFuture<void> future =
      QtConcurrent::run([this, sourcePath, outputPath, quality]() {
        auto progressCallback = [this](int current, int total) {
          QMetaObject::invokeMethod(
              this,
              [this, current, total]() {
                m_progressBar->setRange(0, total);
                m_progressBar->setValue(current);
                m_progressBar->setFormat(
                    QString("Compressing page %v of %m (%p%)"));
              },
              Qt::QueuedConnection);
        };

        bool success =
            compressPdf(sourcePath, outputPath, quality, progressCallback);

        QMetaObject::invokeMethod(
            this,
            [this, sourcePath, success, outputPath]() {
              setProcessingState(false);
              m_cancelButton->setText("Cancel Compression");

              if (success) {
                qint64 origSize = QFileInfo(sourcePath).size();
                qint64 compressedSize = QFileInfo(outputPath).size();
                QString sizeStats;
                if (origSize > 0 && compressedSize > 0) {
                  double diffPct =
                      (1.0 - (static_cast<double>(compressedSize) / origSize)) *
                      100.0;
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

  QFuture<void> future = QtConcurrent::run(
      [this, sourcePaths, outputDir, quality, totalFiles]() {
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
                 QFile::exists(
                     QDir(outputDir).filePath(uniqueBaseName + ".pdf"))) {
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
              this, [this, i]() { m_progressBar->setValue(i + 1); },
              Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(
            this,
            [this, successCount, failureCount, totalOrigBytes,
             totalOutputBytes]() {
              setProcessingState(false);
              m_cancelButton->setText("Cancel Compression");

              QString title = m_isCancelled ? "Batch Compression Cancelled"
                                            : "Batch Compression Complete";
              QString message =
                  QString("%1 PDF(s) compressed successfully.\n%2 PDF(s) failed.")
                      .arg(successCount)
                      .arg(failureCount);

              if (totalOrigBytes > 0 && totalOutputBytes > 0 &&
                  successCount > 0) {
                double diffPct =
                    (1.0 - (static_cast<double>(totalOutputBytes) /
                            totalOrigBytes)) *
                    100.0;
                message +=
                    QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
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

QString PdfPage::findGhostscriptExecutable() {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList bundledCandidates = {
      QDir(appDir).filePath("gs/bin/gswin64c.exe"),
      QDir(appDir).filePath("gswin64c.exe"),
      QDir(appDir).filePath("gs/bin/gs")};
  for (const QString &candidate : bundledCandidates) {
    if (QFileInfo::exists(candidate) && QFileInfo(candidate).isExecutable()) {
      return candidate;
    }
  }

  const QStringList systemNames = {"gswin64c", "gswin32c", "gs"};
  for (const QString &name : systemNames) {
    QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
      return found;
    }
  }
  return QString();
}

bool PdfPage::compressPdfWithGhostscript(const QString &inputPath,
                                         const QString &tempOutputPath,
                                         int quality) {
  QString gsBin = findGhostscriptExecutable();
  if (gsBin.isEmpty()) {
    return false;
  }

  int clampedQuality = qBound(1, quality, 100);
  int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
  QString pdfSetting = (clampedQuality <= 30)   ? "/screen"
                       : (clampedQuality <= 70) ? "/ebook"
                                                : "/printer";

  QStringList args = {
      "-sDEVICE=pdfwrite",
      "-dCompatibilityLevel=1.5",
      QString("-dPDFSETTINGS=%1").arg(pdfSetting),
      "-dNOPAUSE",
      "-dQUIET",
      "-dBATCH",
      "-dDetectDuplicateImages=true",
      "-dCompressFonts=true",
      "-dSubsetFonts=true",
      "-dAutoFilterColorImages=false",
      "-dAutoFilterGrayImages=false",
      "-dColorImageFilter=/DCTEncode",
      "-dGrayImageFilter=/DCTEncode",
      "-dDownsampleColorImages=true",
      "-dDownsampleGrayImages=true",
      "-dDownsampleMonoImages=true",
      "-dColorImageDownsampleType=/Bicubic",
      "-dGrayImageDownsampleType=/Bicubic",
      QString("-dColorImageResolution=%1").arg(targetDpi),
      QString("-dGrayImageResolution=%1").arg(targetDpi),
      QString("-dMonoImageResolution=%1").arg(targetDpi * 2),
      QString("-sOutputFile=%1").arg(QDir::toNativeSeparators(tempOutputPath)),
      QDir::toNativeSeparators(inputPath)};

  QProcess proc;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setStandardOutputFile(QProcess::nullDevice());
  proc.start(gsBin, args);
  if (!proc.waitForStarted(3000)) {
    return false;
  }
  while (!proc.waitForFinished(150)) {
    if (proc.state() == QProcess::NotRunning) {
      break;
    }
    if (m_isCancelled) {
      proc.kill();
      proc.waitForFinished(1000);
      QFile::remove(tempOutputPath);
      return false;
    }
  }

  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0 ||
      !QFileInfo::exists(tempOutputPath) ||
      QFileInfo(tempOutputPath).size() == 0) {
    QFile::remove(tempOutputPath);
    return false;
  }

  return true;
}

bool PdfPage::compressPdfWithQtFallback(
    const QString &inputPath, const QString &tempOutputPath, int quality,
    const std::function<void(int, int)> &onPageProgress) {
  QPdfDocument pdfDocument;
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

  QPdfWriter pdfWriter(tempOutputPath);
  int dpi = calculateDPI(quality);
  pdfWriter.setResolution(dpi);
  pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

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

    // Unified DPI calculation: 1 point = 1/72 inch, so pixel size = pt * dpi / 72.0
    QSize renderSize(qMax(1, qRound(pageSize.width() * dpi / 72.0)),
                     qMax(1, qRound(pageSize.height() * dpi / 72.0)));

    QImage pageImage = pdfDocument.render(i, renderSize);
    if (pageImage.isNull()) {
      qWarning() << "Failed to render page" << i;
      painter.end();
      QFile::remove(tempOutputPath);
      return false;
    }

    pageImage = compressImage(pageImage, quality);

    QRect pageRect =
        pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    painter.drawImage(pageRect, pageImage);

    if (onPageProgress) {
      onPageProgress(i + 1, totalPages);
    }
  }

  painter.end();
  return !m_isCancelled;
}

bool PdfPage::compressPdf(const QString &inputPath, const QString &outputPath,
                          int quality,
                          const std::function<void(int, int)> &onPageProgress) {
  QString tempOutputPath = outputPath + ".tmp";
  QFile::remove(tempOutputPath);

  // 1. Try vector-preserving Ghostscript engine first (like PDF24 / iLovePDF)
  bool ok = compressPdfWithGhostscript(inputPath, tempOutputPath, quality);
  if (ok && onPageProgress) {
    onPageProgress(100, 100);
  }

  // 2. Fallback to corrected Qt PDF engine if Ghostscript is not available
  if (!ok && !m_isCancelled) {
    ok = compressPdfWithQtFallback(inputPath, tempOutputPath, quality,
                                   onPageProgress);
  }

  if (!ok || m_isCancelled) {
    QFile::remove(tempOutputPath);
    return false;
  }

  // 3. Size inflation guard: never output a file larger than the original PDF
  qint64 origBytes = QFileInfo(inputPath).size();
  qint64 newBytes = QFileInfo(tempOutputPath).size();
  if (origBytes > 0 && newBytes >= origBytes) {
    QFile::remove(tempOutputPath);
    if (!QFile::copy(inputPath, tempOutputPath)) {
      return false;
    }
  }

  return replaceDestinationAtomically(tempOutputPath, outputPath);
}

QImage PdfPage::compressImage(const QImage &originalImage, int quality) {
  if (originalImage.isNull()) {
    return originalImage;
  }

  // Paint onto opaque white canvas and convert to RGB888 (drops alpha overhead without noisy dithering)
  QImage rgbImage(originalImage.size(), QImage::Format_RGB888);
  rgbImage.fill(Qt::white);
  {
    QPainter p(&rgbImage);
    p.drawImage(0, 0, originalImage);
  }

  int jpegQuality = qBound(25, quality, 95);
  QByteArray jpegData;
  QBuffer buffer(&jpegData);
  if (buffer.open(QIODevice::WriteOnly)) {
    if (rgbImage.save(&buffer, "JPG", jpegQuality)) {
      buffer.close();
      QImage decoded;
      if (decoded.loadFromData(jpegData, "JPG")) {
        return decoded;
      }
    }
  }

  return rgbImage;
}

int PdfPage::calculateDPI(int quality) {
  int minDPI = 72;
  int maxDPI = 200;
  return minDPI + ((maxDPI - minDPI) * qBound(0, quality, 100)) / 100;
}
