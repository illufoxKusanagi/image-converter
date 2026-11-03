#include "controllers/grayscale_controller.h"
#include "controllers/pdf_utils.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSet>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>
#include <memory>

GrayscaleController::GrayscaleController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<GrayscaleController::Result>();
}

void GrayscaleController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

bool GrayscaleController::convertPdfToGrayscaleWithQtPdf(
    QPdfDocument &sourceDoc, const QString &tempOutputPath, int quality,
    QString &error) {
  const int clampedQuality = qBound(0, quality, 100);
  const int dpi =
      (clampedQuality >= 80) ? 300 : (100 + (200 * clampedQuality) / 80);

  QPdfWriter writer(tempOutputPath);
  writer.setResolution(dpi);
  std::unique_ptr<QPainter> painter;

  for (int page = 0; page < sourceDoc.pageCount(); ++page) {
    if (m_isCancelled) {
      return false;
    }
    const QSizeF ptSize = sourceDoc.pagePointSize(page);
    QPageLayout layout(QPageSize(ptSize, QPageSize::Point),
                       QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                       QPageLayout::Point);
    writer.setPageLayout(layout);
    if (!painter) {
      painter = std::make_unique<QPainter>(&writer);
      if (!painter->isActive()) {
        error = "Failed to initialize PDF writer.";
        return false;
      }
    } else {
      writer.newPage();
    }

    const QSize renderSize(qMax(1, qRound(ptSize.width() * dpi / 72.0)),
                           qMax(1, qRound(ptSize.height() * dpi / 72.0)));
    const QImage colorImg = pdfutil::renderPdfPageOnWhite(sourceDoc, page, renderSize);
    if (colorImg.isNull()) {
      error = QString("Failed to render page %1.").arg(page + 1);
      return false;
    }
    const QImage grayImg = colorImg.convertToFormat(QImage::Format_Grayscale8);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(
        QRectF(0, 0, painter->device()->width(), painter->device()->height()),
        grayImg);
  }

  if (painter) {
    painter->end();
  }
  return QFileInfo(tempOutputPath).size() > 0;
}

CompressController::SingleResult
GrayscaleController::convertPdfToGrayscale(const QString &inputPath,
                                          const QString &outputPath,
                                          int quality) {
  using Status = CompressController::Result::Status;
  CompressController::SingleResult result;
  if (m_isCancelled) {
    result.status = Status::Cancelled;
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
      destinationInfo.absoluteDir().filePath(".pdf-grayscale-XXXXXX"));
  if (!temporaryDirectory.isValid()) {
    result.error = QString("Cannot create temporary output in the destination "
                           "directory: %1")
                       .arg(temporaryDirectory.errorString());
    return result;
  }
  const QString candidatePath = temporaryDirectory.filePath("candidate.pdf");
  const QString executable = pdfutil::findGhostscriptExecutable();

  bool converted = false;
  if (!executable.isEmpty()) {
    converted = CompressController::compressPdfWithGhostscript(
        executable, sourceInfo.absoluteFilePath(), candidatePath, quality,
        result.error, true, &m_isCancelled);
  } else {
    converted = convertPdfToGrayscaleWithQtPdf(sourceDocument, candidatePath,
                                               quality, result.error);
  }

  if (!converted) {
    if (m_isCancelled) {
      result.status = Status::Cancelled;
    }
    return result;
  }

  QPdfDocument candidateDocument;
  const auto candidateError = candidateDocument.load(candidatePath);
  if (candidateError != QPdfDocument::Error::None ||
      candidateDocument.pageCount() != sourceDocument.pageCount()) {
    result.error = "The grayscale PDF failed validation (unreadable output "
                   "or changed page count). No destination was replaced.";
    return result;
  }
  for (int page = 0; page < sourceDocument.pageCount(); ++page) {
    if (m_isCancelled) {
      result.status = Status::Cancelled;
      return result;
    }
    const QSizeF before = sourceDocument.pagePointSize(page);
    const QSizeF after = candidateDocument.pagePointSize(page);
    if (qAbs(before.width() - after.width()) > 0.5 ||
        qAbs(before.height() - after.height()) > 0.5) {
      result.error = "The grayscale PDF changed page dimensions. "
                     "No destination was replaced.";
      return result;
    }
  }
  candidateDocument.close();
  sourceDocument.close();

  if (m_isCancelled) {
    result.status = Status::Cancelled;
    return result;
  }
  const QFileInfo currentSource(sourceInfo.absoluteFilePath());
  if (currentSource.size() != result.originalBytes ||
      currentSource.lastModified() != sourceModified) {
    result.error = "The input PDF changed during conversion. Please retry.";
    return result;
  }
  const qint64 candidateBytes = QFileInfo(candidatePath).size();
  if (candidateBytes <= 0) {
    result.error = "The grayscale PDF is empty. No destination was replaced.";
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
      result.status = Status::Cancelled;
    }
    return result;
  }
  result.status = Status::Compressed;
  result.outputBytes = candidateBytes;
  return result;
}

void GrayscaleController::runJob(const Params &params) {
  Result result;
  result.isBatch = params.isBatch;

  if (!params.isBatch) {
    const QString &sourcePath = params.sourcePath;
    const QString &outputPath = params.outputPath;
    emit progressChanged(0, 1, QString("Converting %1 to grayscale (1 of 1)...")
                                   .arg(QFileInfo(sourcePath).fileName()));
    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Converting PDF to grayscale");

    const CompressController::SingleResult single =
        convertPdfToGrayscale(sourcePath, outputPath, params.quality);
    result.origBytes = single.originalBytes;
    result.outBytes = single.outputBytes;
    result.singleOutputPath = outputPath;

    using Status = CompressController::Result::Status;
    int statusInt = static_cast<int>(FileStatus::Failed);
    QString detail = single.error;
    switch (single.status) {
    case Status::Compressed:
      statusInt = static_cast<int>(FileStatus::Succeeded);
      detail = QString("Saved grayscale PDF to %1").arg(outputPath);
      result.success = true;
      result.successCount = 1;
      break;
    case Status::Cancelled:
      statusInt = static_cast<int>(FileStatus::Cancelled);
      detail = "Conversion cancelled; no output saved";
      result.cancelled = true;
      break;
    case Status::Unchanged:
    case Status::Failed:
      result.failCount = 1;
      result.error = single.error;
      result.errors.append(single.error);
      break;
    }
    emit fileStatusChanged(sourcePath, statusInt, detail);
    if (single.status != Status::Cancelled) {
      emit progressChanged(1, 1, "Completed");
    }
    emit finished(result);
    return;
  }

  // Batch grayscale
  const QStringList &sourcePaths = params.sourcePaths;
  const int totalFiles = sourcePaths.size();
  emit progressChanged(0, totalFiles,
                       QString("Converting 0 of %1 PDFs to grayscale...")
                           .arg(totalFiles));

  QSet<QString> usedBaseNames;

  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled) {
      break;
    }

    const QString &sourcePath = sourcePaths.at(i);
    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Converting PDF to grayscale");
    emit progressChanged(
        i, totalFiles,
        QString("Converting %1 (%2 of %3)...")
            .arg(QFileInfo(sourcePath).fileName())
            .arg(i + 1)
            .arg(totalFiles));

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
    QString baseName = fileInfo.baseName() + "_grayscale";
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

    const CompressController::SingleResult single =
        convertPdfToGrayscale(sourcePath, outputPath, params.quality);

    using Status = CompressController::Result::Status;
    int statusInt = static_cast<int>(FileStatus::Failed);
    QString detail = single.error;
    if (single.status == Status::Compressed) {
      statusInt = static_cast<int>(FileStatus::Succeeded);
      detail = QString("Saved to %1").arg(outputPath);
    } else if (single.status == Status::Cancelled) {
      statusInt = static_cast<int>(FileStatus::Cancelled);
      detail = "Conversion cancelled; no output saved";
    }
    emit fileStatusChanged(sourcePath, statusInt, detail);

    if (single.status != Status::Cancelled) {
      emit progressChanged(i + 1, totalFiles,
                           QString("Processed %1 of %2 PDFs")
                               .arg(i + 1)
                               .arg(totalFiles));
    }

    if (single.status == Status::Cancelled) {
      break;
    }
    if (single.status == Status::Compressed) {
      result.successCount++;
      result.origBytes += single.originalBytes;
      result.outBytes += single.outputBytes;
    } else {
      result.failCount++;
      if (result.errors.size() < 3) {
        result.errors.append(
            QString("%1: %2").arg(fileInfo.fileName(), single.error));
      }
    }
  }

  result.cancelled = m_isCancelled.load();
  result.success = (result.successCount > 0 && result.failCount == 0);
  if (!result.errors.isEmpty()) {
    result.error = result.errors.join("\n");
  }
  emit finished(result);
}

