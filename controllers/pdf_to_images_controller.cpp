#include "controllers/pdf_to_images_controller.h"
#include "controllers/pdf_utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPdfDocument>
#include <QSaveFile>
#include <QSet>
#include <QtConcurrent/QtConcurrent>

PdfToImagesController::PdfToImagesController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<PdfToImagesController::Result>();
}

void PdfToImagesController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

void PdfToImagesController::runJob(const Params &params) {
  Result result;
  result.outputDir = params.outputDir;
  result.format = params.format;
  result.dpi = params.dpi;

  const QStringList &sourcePaths = params.sourcePaths;
  const QString &format = params.format.toLower();
  const int effectiveQuality = (format == "png") ? -1 : params.quality;
  const int dpi = params.dpi;

  emit progressChanged(0, sourcePaths.size(), "Exporting PDF pages to images...");

  int totalExportedImages = 0;
  int failedDocs = 0;
  QStringList errors;
  QSet<QString> usedFileNames;

  for (int i = 0; i < sourcePaths.size(); ++i) {
    if (m_isCancelled) {
      break;
    }
    const QString &sourcePath = sourcePaths.at(i);
    const QFileInfo sourceInfo(sourcePath);

    emit fileStatusChanged(
        sourcePath, static_cast<int>(FileStatus::Processing),
        QString("Rendering pages to %1 at %2 DPI")
            .arg(format.toUpper())
            .arg(dpi));
    emit progressChanged(
        i, sourcePaths.size(),
        QString("Exporting %1 (%2 of %3)...")
            .arg(sourceInfo.fileName())
            .arg(i + 1)
            .arg(sourcePaths.size()));

    QPdfDocument doc;
    if (doc.load(sourceInfo.absoluteFilePath()) != QPdfDocument::Error::None ||
        doc.pageCount() <= 0) {
      failedDocs++;
      const QString err = "Cannot open PDF (damaged or encrypted).";
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed), err);
      continue;
    }

    QVector<int> targetPages;
    if (params.rangeFilter.isEmpty()) {
      for (int p = 1; p <= doc.pageCount(); ++p) {
        targetPages.append(p);
      }
    } else {
      QVector<QPair<int, int>> ranges;
      QString rangeErr;
      if (!pdfutil::parsePageRanges(params.rangeFilter, doc.pageCount(), ranges, rangeErr)) {
        failedDocs++;
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), rangeErr));
        emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed), rangeErr);
        continue;
      }
      for (const auto &r : ranges) {
        for (int p = r.first; p <= r.second; ++p) {
          if (!targetPages.contains(p)) {
            targetPages.append(p);
          }
        }
      }
    }

    int exportedForThisPdf = 0;
    QString docError;
    for (int pageNum : targetPages) {
      if (m_isCancelled) {
        break;
      }
      const int pageIdx = pageNum - 1;
      const QSizeF ptSize = doc.pagePointSize(pageIdx);
      const QSize pixelSize(qMax(1, qRound(ptSize.width() * dpi / 72.0)),
                            qMax(1, qRound(ptSize.height() * dpi / 72.0)));

      const QImage::Format imgFormat =
          (format == "jpg") ? QImage::Format_RGB32 : QImage::Format_ARGB32;
      const QImage pageImg =
          pdfutil::renderPdfPageOnWhite(doc, pageIdx, pixelSize, imgFormat);
      if (pageImg.isNull()) {
        docError = QString("Failed to render page %1.").arg(pageNum);
        break;
      }

      const QString baseStem = QString("%1_page_%2")
                                   .arg(sourceInfo.baseName())
                                   .arg(pageNum, 3, 10, QLatin1Char('0'));
      QString uniqueStem = baseStem;
      int suffix = 1;
      while (usedFileNames.contains(uniqueStem.toLower()) ||
             QFile::exists(
                 QDir(params.outputDir).filePath(uniqueStem + "." + format))) {
        uniqueStem = QString("%1_%2").arg(baseStem).arg(suffix++);
      }
      usedFileNames.insert(uniqueStem.toLower());
      const QString outImagePath =
          QDir(params.outputDir).filePath(uniqueStem + "." + format);

      QSaveFile saveFile(outImagePath);
      saveFile.setDirectWriteFallback(false);
      if (!saveFile.open(QIODevice::WriteOnly) ||
          !pageImg.save(&saveFile, format.toUpper().toUtf8().constData(),
                        effectiveQuality) ||
          !saveFile.commit()) {
        docError = QString("Failed to save image '%1'.")
                       .arg(QFileInfo(outImagePath).fileName());
        break;
      }
      exportedForThisPdf++;
      totalExportedImages++;
    }

    doc.close();

    if (m_isCancelled) {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Cancelled),
                             "Export cancelled");
    } else if (docError.isEmpty() && exportedForThisPdf > 0) {
      emit fileStatusChanged(
          sourcePath, static_cast<int>(FileStatus::Succeeded),
          QString("Exported %1 %2 image(s) to %3")
              .arg(exportedForThisPdf)
              .arg(format.toUpper())
              .arg(params.outputDir));
    } else {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             docError);
    }

    if (!m_isCancelled) {
      emit progressChanged(
          i + 1, sourcePaths.size(),
          QString("Exported %1 of %2").arg(i + 1).arg(sourcePaths.size()));
    }

    if (!docError.isEmpty() && !m_isCancelled) {
      failedDocs++;
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), docError));
    }
  }

  result.success = (totalExportedImages > 0 && failedDocs == 0);
  result.cancelled = m_isCancelled.load();
  result.imagesExported = totalExportedImages;
  result.failedDocs = failedDocs;
  result.errors = errors;
  if (!errors.isEmpty()) {
    result.error = errors.join("\n");
  }

  emit finished(result);
}

