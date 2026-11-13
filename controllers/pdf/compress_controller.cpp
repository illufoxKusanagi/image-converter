#include "controllers/pdf/compress_controller.h"
#include "controllers/pdf/pdf_utils.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QPdfDocument>
#include <QSet>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>

CompressController::CompressController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<CompressController::Result>();
}

void CompressController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

bool CompressController::compressPdfWithGhostscript(
    const QString &executable, const QString &inputPath,
    const QString &tempOutputPath, int quality, QString &error, bool grayscale,
    const std::atomic<bool> *cancelled) {
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
                      "-dDetectDuplicateImages=true",
                      "-dCompressFonts=true",
                      "-dCompressStreams=true",
                      "-dSubsetFonts=true",
                      "-dPreserveAnnots=true",
                      "-dPreserveMarkedContent=true",
                      "-dWantsOptionalContent=true",
                      "-dDownsampleMonoImages=false"};

  if (grayscale) {
    args << "-sColorConversionStrategy=Gray"
         << "-dProcessColorModel=/DeviceGray";
  } else {
    args << "-sColorConversionStrategy=LeaveColorUnchanged"
         << "-dPassThroughJPEGImages=true"
         << "-dPassThroughJPXImages=true";
  }

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
  args << "-sOutputFile=candidate.pdf" << "-c" << distillerParameters << "-f"
       << QDir::toNativeSeparators(inputPath);

  std::atomic<bool> defaultCancelled{false};
  const std::atomic<bool> &cancelRef = cancelled ? *cancelled : defaultCancelled;
  return pdfutil::runGhostscriptProcess(executable, args,
                                        QFileInfo(tempOutputPath).absolutePath(),
                                        tempOutputPath, cancelRef, error, true);
}

CompressController::SingleResult
CompressController::compressPdf(const QString &inputPath,
                                const QString &outputPath, int quality) {
  SingleResult result;
  if (m_isCancelled) {
    result.status = Result::Status::Cancelled;
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
  if (pdfutil::isSamePdf(sourceInfo, destinationInfo)) {
    result.error = "Choose a different output file. The original PDF must not "
                   "be overwritten.";
    return result;
  }

  const QString executable = pdfutil::findGhostscriptExecutable();
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
                                  candidatePath, quality, result.error,
                                  false, &m_isCancelled)) {
    if (m_isCancelled) {
      result.status = Result::Status::Cancelled;
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
      result.status = Result::Status::Cancelled;
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
    result.status = Result::Status::Cancelled;
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
    result.status = Result::Status::Unchanged;
    result.outputBytes = result.originalBytes;
    return result;
  }

  if (pdfutil::isSamePdf(currentSource, QFileInfo(destinationInfo.absoluteFilePath()))) {
    result.error = "The destination now points to the input PDF. "
                   "No destination was replaced.";
    return result;
  }
  if (!pdfutil::savePdfAtomically(candidatePath, destinationInfo.absoluteFilePath(),
                                  m_isCancelled, result.error)) {
    if (m_isCancelled) {
      result.status = Result::Status::Cancelled;
    }
    return result;
  }
  result.status = Result::Status::Compressed;
  result.outputBytes = candidateBytes;
  return result;
}

void CompressController::runJob(const Params &params) {
  Result result;
  result.isBatch = params.isBatch;

  if (!params.isBatch) {
    const QString &sourcePath = params.sourcePath;
    const QString &outputPath = params.outputPath;
    emit progressChanged(0, 1, QString("Compressing %1 (1 of 1)...")
                                   .arg(QFileInfo(sourcePath).fileName()));
    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Optimizing PDF with Ghostscript");

    const SingleResult single = compressPdf(sourcePath, outputPath, params.quality);
    result.status = single.status;
    result.origBytes = single.originalBytes;
    result.outBytes = single.outputBytes;
    result.singleOutputPath = outputPath;

    int statusInt = static_cast<int>(FileStatus::Failed);
    QString detail = single.error;
    switch (single.status) {
    case Result::Status::Compressed:
      statusInt = static_cast<int>(FileStatus::Succeeded);
      detail = QString("Saved to %1").arg(outputPath);
      result.successCount = 1;
      break;
    case Result::Status::Unchanged:
      statusInt = static_cast<int>(FileStatus::Unchanged);
      detail = "No smaller PDF produced; no output saved";
      result.unchangedCount = 1;
      break;
    case Result::Status::Cancelled:
      statusInt = static_cast<int>(FileStatus::Cancelled);
      detail = "Compression cancelled; no output saved";
      break;
    case Result::Status::Failed:
      result.failCount = 1;
      result.errors.append(single.error);
      break;
    }
    emit fileStatusChanged(sourcePath, statusInt, detail);
    if (single.status != Result::Status::Cancelled) {
      emit progressChanged(1, 1, "Completed");
    }
    emit finished(result);
    return;
  }

  // Batch compression
  const QStringList &sourcePaths = params.sourcePaths;
  const int totalFiles = sourcePaths.size();
  emit progressChanged(0, totalFiles,
                       QString("Compressing 0 of %1 PDFs...").arg(totalFiles));

  QSet<QString> usedBaseNames;

  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled) {
      break;
    }

    const QString &sourcePath = sourcePaths.at(i);
    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Optimizing PDF with Ghostscript");
    emit progressChanged(
        i, totalFiles,
        QString("Compressing %1 (%2 of %3; %4 completed)...")
            .arg(QFileInfo(sourcePath).fileName())
            .arg(i + 1)
            .arg(totalFiles)
            .arg(i));

    auto collidesWithAnySource = [&](const QString &candidateOut) {
      const QFileInfo candInfo(candidateOut);
      for (const QString &src : sourcePaths) {
        if (pdfutil::isSamePdf(QFileInfo(src), candInfo)) {
          return true;
        }
      }
      return false;
    };

    QFileInfo fileInfo(sourcePath);
    QString baseName = fileInfo.baseName() + "_compressed";
    QString uniqueBaseName = baseName;
    int suffix = 1;
    while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
           QFile::exists(QDir(params.outputDir).filePath(uniqueBaseName + ".pdf")) ||
           collidesWithAnySource(QDir(params.outputDir).filePath(uniqueBaseName + ".pdf"))) {
      uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
    }
    usedBaseNames.insert(uniqueBaseName.toLower());

    const QString outputPath =
        QDir(params.outputDir).filePath(uniqueBaseName + ".pdf");

    const SingleResult single = compressPdf(sourcePath, outputPath, params.quality);
    int statusInt = static_cast<int>(FileStatus::Failed);
    QString detail = single.error;
    switch (single.status) {
    case Result::Status::Compressed:
      statusInt = static_cast<int>(FileStatus::Succeeded);
      detail = QString("Saved to %1").arg(outputPath);
      break;
    case Result::Status::Unchanged:
      statusInt = static_cast<int>(FileStatus::Unchanged);
      detail = "No smaller PDF produced; no output saved";
      break;
    case Result::Status::Cancelled:
      statusInt = static_cast<int>(FileStatus::Cancelled);
      detail = "Compression cancelled; no output saved";
      break;
    case Result::Status::Failed:
      break;
    }
    emit fileStatusChanged(sourcePath, statusInt, detail);

    if (single.status != Result::Status::Cancelled) {
      emit progressChanged(i + 1, totalFiles,
                           QString("Processed %1 of %2 PDFs")
                               .arg(i + 1)
                               .arg(totalFiles));
    }

    if (single.status == Result::Status::Cancelled) {
      break;
    }
    if (single.status == Result::Status::Compressed) {
      result.successCount++;
      result.origBytes += single.originalBytes;
      result.outBytes += single.outputBytes;
    } else if (single.status == Result::Status::Unchanged) {
      result.unchangedCount++;
    } else {
      result.failCount++;
      if (result.errors.size() < 3) {
        result.errors.append(
            QString("%1: %2").arg(fileInfo.fileName(), single.error));
      }
    }
  }

  if (m_isCancelled) {
    result.status = Result::Status::Cancelled;
  } else {
    result.status = (result.failCount == 0) ? Result::Status::Compressed
                                            : Result::Status::Failed;
  }
  emit finished(result);
}
