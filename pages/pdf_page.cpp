#include "pdf_page.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QPdfDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>

namespace {
bool isSamePdf(const QFileInfo &source, const QFileInfo &destination) {
  const QString canonicalSource = source.canonicalFilePath();
  const QString canonicalDestination = destination.canonicalFilePath();
#ifdef Q_OS_WIN
  constexpr Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
  constexpr Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
  return source == destination ||
         (!canonicalSource.isEmpty() && !canonicalDestination.isEmpty() &&
          canonicalSource.compare(canonicalDestination, pathCase) == 0);
}

bool savePdfAtomically(const QString &candidatePath, const QString &outputPath,
                       const std::atomic<bool> &cancelled, QString &error) {
  QFile candidate(candidatePath);
  QSaveFile destination(outputPath);
  // Never fall back to truncating an existing destination on a failed save.
  destination.setDirectWriteFallback(false);
  if (!candidate.open(QIODevice::ReadOnly)) {
    error = QString("Cannot read the compressed PDF: %1")
                .arg(candidate.errorString());
    return false;
  }
  if (!destination.open(QIODevice::WriteOnly)) {
    error = QString("Cannot save the compressed PDF: %1")
                .arg(destination.errorString());
    return false;
  }

  while (!candidate.atEnd()) {
    if (cancelled) {
      return false;
    }
    const QByteArray chunk = candidate.read(1024 * 1024);
    if (candidate.error() != QFileDevice::NoError) {
      error = QString("Cannot read the compressed PDF: %1")
                  .arg(candidate.errorString());
      return false;
    }
    if (destination.write(chunk) != chunk.size()) {
      error = QString("Cannot save the compressed PDF: %1")
                  .arg(destination.errorString());
      return false;
    }
  }
  if (cancelled) {
    return false;
  }
  if (!destination.commit()) {
    error = QString("Cannot commit the compressed PDF: %1")
                .arg(destination.errorString());
    return false;
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

  QStringList presetOptions = {"Custom", "Web / Email", "Balanced",
                               "High Quality (preserve images)"};
  m_presetSelect = new ui::Select(presetOptions, this);
  m_presetSelect->setCurrentIndex(2); // Default to Balanced (50)
  m_presetField = new ui::FormField("Compression Preset", m_presetSelect, this);
  m_presetField->setToolTip(
      "Text and vector graphics are retained. Below quality 80, images may "
      "lose detail. At quality 80 or above, images are not downsampled or "
      "newly JPEG-encoded. PDF rewriting can alter forms and annotations "
      "and invalidates digital signatures. Keep the original PDF.");

  connect(m_presetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfPage::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfPage::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_presetField, 1, Qt::AlignTop);

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignCenter);
  m_progressLabel->setVisible(false);

  m_progressBar = new ui::Progress(0, this);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Compress PDF", ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);

  m_cancelButton =
      new ui::Button("Cancel Compression", ui::ButtonVariant::Destructive,
                     ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &PdfPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  mainLayout->addLayout(attributeLayout);
  mainLayout->addWidget(m_progressLabel);
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
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

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
  if (!m_qualitySlider)
    return;
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
  if (!m_presetSelect || !m_qualitySlider)
    return;
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
  if (m_progressLabel) {
    m_progressLabel->setVisible(isProcessing);
  }
  if (m_progressBar) {
    m_progressBar->setVisible(isProcessing);
  }
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
    ui::Toast::show(this, "No PDF Selected",
                    "Please select at least one PDF file.",
                    ui::ToastVariant::Warning);
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
  m_progressBar->setRange(0, 0);
  m_progressLabel->setText("Optimizing PDF with Ghostscript...");

  QFuture<void> future = QtConcurrent::run([this, sourcePath, outputPath,
                                            quality]() {
    const CompressionResult result =
        compressPdf(sourcePath, outputPath, quality);

    QMetaObject::invokeMethod(
        this,
        [this, result, outputPath]() {
          setProcessingState(false);
          m_cancelButton->setText("Cancel Compression");

          if (result.status == CompressionStatus::Compressed) {
            const double savedPercent =
                100.0 * (1.0 - static_cast<double>(result.outputBytes) /
                                   result.originalBytes);
            ui::Dialog::info(
                this, "PDF Compressed",
                QString("Saved to: %1\n\nOriginal: %2\nCompressed: %3 "
                        "(%4% smaller)")
                    .arg(outputPath)
                    .arg(DropFileWidget::formatFileSize(result.originalBytes))
                    .arg(DropFileWidget::formatFileSize(result.outputBytes))
                    .arg(savedPercent, 0, 'f', 1));
          } else if (result.status == CompressionStatus::Unchanged) {
            ui::Dialog::info(
                this, "No Reduction Achieved",
                "These settings did not produce a smaller PDF. "
                "The original PDF and destination were left unchanged.");
          } else if (result.status == CompressionStatus::Cancelled) {
            ui::Toast::show(this, "Cancelled", "PDF compression was cancelled.",
                            ui::ToastVariant::Default);
          } else {
            ui::Toast::show(this, "PDF Compression Failed", result.error,
                            ui::ToastVariant::Destructive);
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
  m_progressLabel->setText(
      QString("Compressing 0 of %1 PDFs...").arg(totalFiles));

  QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                            quality, totalFiles]() {
    int successCount = 0;
    int unchangedCount = 0;
    int failureCount = 0;
    qint64 totalOrigBytes = 0;
    qint64 totalOutputBytes = 0;
    QStringList errors;
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

      QString outputPath = QDir(outputDir).filePath(uniqueBaseName + ".pdf");

      const CompressionResult result =
          compressPdf(sourcePath, outputPath, quality);
      if (result.status == CompressionStatus::Cancelled) {
        break;
      }
      if (result.status == CompressionStatus::Compressed) {
        successCount++;
        totalOrigBytes += result.originalBytes;
        totalOutputBytes += result.outputBytes;
      } else if (result.status == CompressionStatus::Unchanged) {
        unchangedCount++;
      } else {
        failureCount++;
        if (errors.size() < 3) {
          errors.append(
              QString("%1: %2").arg(fileInfo.fileName(), result.error));
        }
      }

      QMetaObject::invokeMethod(
          this,
          [this, i, totalFiles]() {
            m_progressBar->setValue(i + 1);
            m_progressLabel->setText(QString("Compressing %1 of %2 PDFs...")
                                         .arg(i + 1)
                                         .arg(totalFiles));
          },
          Qt::QueuedConnection);
    }

    QMetaObject::invokeMethod(
        this,
        [this, successCount, unchangedCount, failureCount, totalFiles,
         totalOrigBytes, totalOutputBytes, errors]() {
          setProcessingState(false);
          m_cancelButton->setText("Cancel Compression");

          QString title = m_isCancelled ? "Batch Compression Cancelled"
                                        : "Batch Compression Complete";
          QString message =
              QString("%1 PDF(s) compressed.\n%2 PDF(s) unchanged (no output "
                      "saved).\n%3 PDF(s) failed.")
                  .arg(successCount)
                  .arg(unchangedCount)
                  .arg(failureCount);

          if (totalOrigBytes > 0 && totalOutputBytes > 0 && successCount > 0) {
            double diffPct = (1.0 - (static_cast<double>(totalOutputBytes) /
                                     totalOrigBytes)) *
                             100.0;
            message +=
                QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
                    .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                    .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                    .arg(qAbs(diffPct), 0, 'f', 1)
                    .arg(diffPct >= 0 ? "smaller" : "larger");
          }

          if (!errors.isEmpty()) {
            message += "\n\n" + errors.join("\n");
          }
          if (m_isCancelled) {
            message += QString("\n%1 PDF(s) not completed due to cancellation.")
                           .arg(totalFiles - successCount - unchangedCount -
                                failureCount);
          }
          ui::Dialog::info(this, title, message);
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

QString PdfPage::findGhostscriptExecutable() {
  // Single-file packagers can point to their extracted runtime explicitly.
  const QString configured = qEnvironmentVariable("IMAGECONVERTER_GHOSTSCRIPT");
  if (!configured.isEmpty()) {
    const QFileInfo executable(configured);
    return executable.isFile() && executable.isExecutable()
               ? executable.absoluteFilePath()
               : QString();
  }

  const QDir appDir(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
  const QStringList bundledNames = {"gs/bin/gswin64c.exe", "gswin64c.exe"};
  const QStringList systemNames = {"gswin64c", "gswin32c"};
#else
  const QStringList bundledNames = {"gs/bin/gs"};
  const QStringList systemNames = {"gs"};
#endif
  for (const QString &name : bundledNames) {
    const QFileInfo executable(appDir.filePath(name));
    if (executable.isFile() && executable.isExecutable()) {
      return executable.absoluteFilePath();
    }
  }
  for (const QString &name : systemNames) {
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
      return found;
    }
  }
  return QString();
}

bool PdfPage::compressPdfWithGhostscript(const QString &executable,
                                         const QString &inputPath,
                                         const QString &tempOutputPath,
                                         int quality, QString &error) {
  const int clampedQuality = qBound(0, quality, 100);
  const bool preserveImages = clampedQuality >= 80;
  QStringList args = {"-sDEVICE=pdfwrite",
                      "-dCompatibilityLevel=1.7",
                      "-dSAFER",
                      "-dPDFSTOPONERROR",
                      "-dNOPAUSE",
                      "-dQUIET",
                      "-dBATCH",
                      "-dAutoRotatePages=/None",
                      "-sColorConversionStrategy=LeaveColorUnchanged",
                      "-dDetectDuplicateImages=true",
                      "-dCompressFonts=true",
                      "-dCompressStreams=true",
                      "-dSubsetFonts=true",
                      "-dPreserveAnnots=true",
                      "-dPreserveMarkedContent=true",
                      "-dWantsOptionalContent=true",
                      "-dPassThroughJPEGImages=true",
                      "-dPassThroughJPXImages=true",
                      "-dDownsampleMonoImages=false"};

  if (preserveImages) {
    args << "-dDownsampleColorImages=false" << "-dDownsampleGrayImages=false"
         << "-dAutoFilterColorImages=false" << "-dAutoFilterGrayImages=false"
         << "-dColorImageFilter=/FlateEncode"
         << "-dGrayImageFilter=/FlateEncode";
  } else {
    const int targetDpi = 100 + (200 * clampedQuality) / 80;
    args << "-dDownsampleColorImages=true" << "-dDownsampleGrayImages=true"
         << "-dColorImageDownsampleType=/Bicubic"
         << "-dGrayImageDownsampleType=/Bicubic"
         << "-dColorImageDownsampleThreshold=1.5"
         << "-dGrayImageDownsampleThreshold=1.5"
         << QString("-dColorImageResolution=%1").arg(targetDpi)
         << QString("-dGrayImageResolution=%1").arg(targetDpi)
         << "-dAutoFilterColorImages=true" << "-dAutoFilterGrayImages=true";
  }

  // QFactor controls pdfwrite's JPEG encoder; -dJPEGQ is not its quality API.
  // Avoid chroma subsampling to keep colored edges and scanned text clearer.
  const QString imageParameters =
      QString("<< /QFactor %1 /Blend 1 /ColorTransform 1 "
              "/HSamples [1 1 1 1] /VSamples [1 1 1 1] >>")
          .arg(0.6 - 0.45 * clampedQuality / 80.0, 0, 'f', 3);
  const QString distillerParameters =
      preserveImages
          ? QString("<< /NeverEmbed [] >> setdistillerparams")
          : QString("<< /NeverEmbed [] /ColorImageDict %1 /GrayImageDict %1 "
                    "/ColorACSImageDict %1 /GrayACSImageDict %1 >> "
                    "setdistillerparams")
                .arg(imageParameters);
  // A fixed relative output name avoids Ghostscript's %d filename expansion
  // even when the user-selected destination directory contains percent signs.
  args << "-sOutputFile=candidate.pdf" << "-c" << distillerParameters << "-f"
       << QDir::toNativeSeparators(inputPath);

  QProcess proc;
  proc.setWorkingDirectory(QFileInfo(tempOutputPath).absolutePath());
  proc.setProcessChannelMode(QProcess::MergedChannels);
  QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
  environment.remove("GS_OPTIONS");
  proc.setProcessEnvironment(environment);
  proc.start(executable, args);
  if (!proc.waitForStarted(5000)) {
    error = QString("Cannot start Ghostscript: %1. Check the bundled runtime "
                    "and its DLLs/resources, or reinstall Ghostscript.")
                .arg(proc.errorString());
    return false;
  }

  QElapsedTimer timer;
  timer.start();
  QByteArray diagnostics;
  while (proc.state() != QProcess::NotRunning) {
    proc.waitForFinished(150);
    diagnostics = (diagnostics + proc.readAll()).right(16384);
    if (m_isCancelled || timer.elapsed() > 15 * 60 * 1000) {
      proc.kill();
      proc.waitForFinished(3000);
      if (!m_isCancelled) {
        error = "Ghostscript exceeded the 15-minute processing limit.";
      }
      return false;
    }
  }
  diagnostics = (diagnostics + proc.readAll()).right(16384);
  if (m_isCancelled) {
    return false;
  }
  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
    error = QString("Ghostscript failed (exit %1): %2")
                .arg(proc.exitCode())
                .arg(QString::fromLocal8Bit(diagnostics).trimmed().right(4096));
    qWarning().noquote() << error;
    return false;
  }
  if (!diagnostics.trimmed().isEmpty()) {
    error = QString("Ghostscript reported warnings; the output was not saved "
                    "to avoid accepting missing or substituted content: %1")
                .arg(QString::fromLocal8Bit(diagnostics).trimmed().right(4096));
    qWarning().noquote() << error;
    return false;
  }
  if (QFileInfo(tempOutputPath).size() <= 0) {
    error = "Ghostscript did not produce a nonempty PDF.";
    return false;
  }
  return true;
}

PdfPage::CompressionResult PdfPage::compressPdf(const QString &inputPath,
                                                const QString &outputPath,
                                                int quality) {
  CompressionResult result;
  if (m_isCancelled) {
    result.status = CompressionStatus::Cancelled;
    return result;
  }

  const QFileInfo sourceInfo(inputPath);
  const QFileInfo destinationInfo(outputPath);
  result.originalBytes = sourceInfo.size();
  const QDateTime sourceModified = sourceInfo.lastModified();
  if (!sourceInfo.isFile() || result.originalBytes <= 0) {
    result.error = "The input PDF is missing or empty.";
    return result;
  }
  if (isSamePdf(sourceInfo, destinationInfo)) {
    result.error = "Choose a different output file. The original PDF must not "
                   "be overwritten.";
    return result;
  }

  const QString executable = findGhostscriptExecutable();
  if (executable.isEmpty()) {
    result.error = "Ghostscript is required for PDF compression. Install the "
                   "ghostscript package on Linux or include the complete "
                   "gs runtime beside the Windows application. For an "
                   "extracted runtime, set IMAGECONVERTER_GHOSTSCRIPT to its "
                   "executable's full path. No PDF was changed.";
    return result;
  }

  QPdfDocument sourceDocument;
  const auto sourceError = sourceDocument.load(sourceInfo.absoluteFilePath());
  if (sourceError != QPdfDocument::Error::None ||
      sourceDocument.pageCount() <= 0) {
    result.error = QString("Cannot open the input PDF (PDF error %1). "
                           "Check whether it is damaged or password-protected.")
                       .arg(static_cast<int>(sourceError));
    return result;
  }

  QTemporaryDir temporaryDirectory(
      destinationInfo.absoluteDir().filePath(".pdf-compress-XXXXXX"));
  if (!temporaryDirectory.isValid()) {
    result.error = QString("Cannot create temporary output in the destination "
                           "directory: %1")
                       .arg(temporaryDirectory.errorString());
    return result;
  }
  const QString candidatePath = temporaryDirectory.filePath("candidate.pdf");
  if (!compressPdfWithGhostscript(executable, sourceInfo.absoluteFilePath(),
                                  candidatePath, quality, result.error)) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
    }
    return result;
  }

  QPdfDocument candidateDocument;
  const auto candidateError = candidateDocument.load(candidatePath);
  if (candidateError != QPdfDocument::Error::None ||
      candidateDocument.pageCount() != sourceDocument.pageCount()) {
    result.error = "The compressed PDF failed validation (unreadable output "
                   "or changed page count). No destination was replaced.";
    return result;
  }
  for (int page = 0; page < sourceDocument.pageCount(); ++page) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
      return result;
    }
    const QSizeF before = sourceDocument.pagePointSize(page);
    const QSizeF after = candidateDocument.pagePointSize(page);
    if (qAbs(before.width() - after.width()) > 0.5 ||
        qAbs(before.height() - after.height()) > 0.5) {
      result.error = "The compressed PDF changed page dimensions. "
                     "No destination was replaced.";
      return result;
    }
  }
  candidateDocument.close();
  sourceDocument.close();

  if (m_isCancelled) {
    result.status = CompressionStatus::Cancelled;
    return result;
  }
  const QFileInfo currentSource(sourceInfo.absoluteFilePath());
  if (currentSource.size() != result.originalBytes ||
      currentSource.lastModified() != sourceModified) {
    result.error = "The input PDF changed during compression. Please retry.";
    return result;
  }
  const qint64 candidateBytes = QFileInfo(candidatePath).size();
  if (candidateBytes <= 0) {
    result.error = "The compressed PDF is empty. No destination was replaced.";
    return result;
  }
  if (candidateBytes >= result.originalBytes) {
    result.status = CompressionStatus::Unchanged;
    result.outputBytes = result.originalBytes;
    return result;
  }

  if (isSamePdf(currentSource, QFileInfo(destinationInfo.absoluteFilePath()))) {
    result.error = "The destination now points to the input PDF. "
                   "No destination was replaced.";
    return result;
  }
  if (!savePdfAtomically(candidatePath, destinationInfo.absoluteFilePath(),
                         m_isCancelled, result.error)) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
    }
    return result;
  }
  result.status = CompressionStatus::Compressed;
  result.outputBytes = candidateBytes;
  return result;
}
