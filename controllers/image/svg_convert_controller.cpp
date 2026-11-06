#include "controllers/image/svg_convert_controller.h"
#include "controllers/pdf/pdf_utils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMarginsF>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QRectF>
#include <QSaveFile>
#include <QSet>
#include <QSvgRenderer>
#include <QtConcurrent/QtConcurrent>

SvgConvertController::SvgConvertController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<SvgConvertController::Result>();
}

void SvgConvertController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

void SvgConvertController::runJob(const Params &params) {
  Result result;
  result.outputDir = params.outputDir;
  const QString format = params.format.trimmed().toLower();
  result.format = format;

  const QStringList &sourcePaths = params.sourcePaths;
  const int totalFiles = sourcePaths.size();
  const int effectiveQuality =
      (format == "png") ? -1 : qBound(1, params.quality, 100);
  const bool useWhiteBg = (format == "jpg" || format == "jpeg" || params.whiteBackground);

  emit progressChanged(0, totalFiles, "Converting SVG vector files...");

  int filesConverted = 0;
  int failedFiles = 0;
  QStringList errors;
  QSet<QString> usedFileNames;

  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled) {
      for (int rem = i; rem < totalFiles; ++rem) {
        emit fileStatusChanged(sourcePaths.at(rem),
                               static_cast<int>(FileStatus::Cancelled),
                               "Conversion cancelled");
      }
      break;
    }

    const QString &sourcePath = sourcePaths.at(i);
    const QFileInfo sourceInfo(sourcePath);

    emit fileStatusChanged(
        sourcePath, static_cast<int>(FileStatus::Processing),
        QString("Rendering SVG to %1").arg(format.toUpper()));
    emit progressChanged(
        i, totalFiles,
        QString("Converting %1 (%2 of %3)...")
            .arg(sourceInfo.fileName())
            .arg(i + 1)
            .arg(totalFiles));

    QSvgRenderer renderer(sourceInfo.absoluteFilePath());
    if (!renderer.isValid()) {
      failedFiles++;
      const QString err = "Invalid or unreadable SVG file.";
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             err);
      continue;
    }

    QSize baseSize = renderer.defaultSize();
    if (!baseSize.isValid() || baseSize.width() <= 0 || baseSize.height() <= 0) {
      baseSize = renderer.viewBoxF().size().toSize();
    }
    if (!baseSize.isValid() || baseSize.width() <= 0 || baseSize.height() <= 0) {
      baseSize = QSize(512, 512);
    }

    QSize targetSize = baseSize;
    if (params.scaleMode == 1) {
      targetSize = baseSize * 2;
    } else if (params.scaleMode == 2) {
      targetSize = baseSize * 4;
    } else if (params.scaleMode == 3) {
      targetSize = QSize(qBound(1, params.customWidth, 16384),
                         qBound(1, params.customHeight, 16384));
    }

    if (targetSize.width() > 16384 || targetSize.height() > 16384) {
      targetSize.scale(16384, 16384, Qt::KeepAspectRatio);
    }
    targetSize.setWidth(qBound(1, targetSize.width(), 16384));
    targetSize.setHeight(qBound(1, targetSize.height(), 16384));

    QRectF renderRect(0.0, 0.0, targetSize.width(), targetSize.height());
    if (params.preserveAspectRatio && baseSize.width() > 0 &&
        baseSize.height() > 0) {
      const qreal scale =
          qMin(static_cast<qreal>(targetSize.width()) / baseSize.width(),
               static_cast<qreal>(targetSize.height()) / baseSize.height());
      const qreal drawW = baseSize.width() * scale;
      const qreal drawH = baseSize.height() * scale;
      renderRect = QRectF((targetSize.width() - drawW) / 2.0,
                          (targetSize.height() - drawH) / 2.0, drawW, drawH);
    }

    const QString baseStem = sourceInfo.completeBaseName().isEmpty()
                                 ? QStringLiteral("vector")
                                 : sourceInfo.completeBaseName();
    QString uniqueStem = baseStem;
    int suffix = 1;
    while (true) {
      const QString candidateName = uniqueStem + "." + format;
      const QString candidatePath = QDir(params.outputDir).filePath(candidateName);
      const QFileInfo candidateInfo(candidatePath);
      const bool collidesWithSource =
          pdfutil::isSamePdf(sourceInfo, candidateInfo);
      if (!usedFileNames.contains(candidateName.toLower()) &&
          !QFile::exists(candidatePath) && !collidesWithSource) {
        break;
      }
      uniqueStem = QString("%1_%2").arg(baseStem).arg(suffix++);
    }
    const QString outFileName = uniqueStem + "." + format;
    usedFileNames.insert(outFileName.toLower());
    const QString outPath = QDir(params.outputDir).filePath(outFileName);

    QString fileError;

    if (format == "pdf") {
      const QString tempPath =
          outPath + QString(".svg_tmp_%1_%2")
                        .arg(QCoreApplication::applicationPid())
                        .arg(QDateTime::currentMSecsSinceEpoch());
      {
        QPdfWriter writer(tempPath);
        writer.setResolution(72);
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));
        writer.setPageSize(QPageSize(QSizeF(targetSize), QPageSize::Point,
                                     QString(), QPageSize::ExactMatch));

        QPainter painter;
        if (!painter.begin(&writer)) {
          fileError = "Failed to initialize PDF writer.";
        } else {
          painter.setRenderHint(QPainter::Antialiasing, true);
          painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
          if (useWhiteBg) {
            painter.fillRect(
                QRectF(0.0, 0.0, targetSize.width(), targetSize.height()),
                Qt::white);
          }
          renderer.render(&painter, renderRect);
          if (!painter.end()) {
            fileError = "Failed to finalize PDF rendering.";
          }
        }
      }

      if (fileError.isEmpty() && !m_isCancelled) {
        QPdfDocument verifyDoc;
        if (verifyDoc.load(tempPath) != QPdfDocument::Error::None ||
            verifyDoc.pageCount() <= 0) {
          fileError = "Generated PDF failed validation.";
        } else {
          verifyDoc.close();
          if (!pdfutil::savePdfAtomically(tempPath, outPath, m_isCancelled,
                                          fileError)) {
            if (fileError.isEmpty() && !m_isCancelled) {
              fileError = "Failed to commit PDF file.";
            }
          }
        }
      }
      QFile::remove(tempPath);
    } else {
      const QImage::Format imgFormat =
          useWhiteBg ? QImage::Format_RGB32
                     : QImage::Format_ARGB32_Premultiplied;
      QImage img(targetSize, imgFormat);
      if (img.isNull()) {
        fileError = "Failed to allocate raster image buffer.";
      } else {
        img.fill(useWhiteBg ? Qt::white : Qt::transparent);
        {
          QPainter painter(&img);
          painter.setRenderHint(QPainter::Antialiasing, true);
          painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
          renderer.render(&painter, renderRect);
        }

        if (!m_isCancelled) {
          QSaveFile saveFile(outPath);
          saveFile.setDirectWriteFallback(false);
          const QByteArray fmtBytes =
              (format == "jpg") ? QByteArray("JPG") : format.toUpper().toUtf8();
          if (!saveFile.open(QIODevice::WriteOnly) ||
              !img.save(&saveFile, fmtBytes.constData(), effectiveQuality) ||
              !saveFile.commit()) {
            fileError =
                QString("Failed to save '%1'.").arg(outFileName);
          }
        }
      }
    }

    if (m_isCancelled) {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Cancelled),
                             "Conversion cancelled");
      for (int rem = i + 1; rem < totalFiles; ++rem) {
        emit fileStatusChanged(sourcePaths.at(rem),
                               static_cast<int>(FileStatus::Cancelled),
                               "Conversion cancelled");
      }
      break;
    }

    if (fileError.isEmpty()) {
      filesConverted++;
      emit fileStatusChanged(
          sourcePath, static_cast<int>(FileStatus::Succeeded),
          QString("Saved %1 (%2×%3)")
              .arg(outFileName)
              .arg(targetSize.width())
              .arg(targetSize.height()));
    } else {
      failedFiles++;
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), fileError));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             fileError);
    }

    emit progressChanged(
        i + 1, totalFiles,
        QString("Converted %1 of %2").arg(i + 1).arg(totalFiles));
  }

  result.success = (filesConverted > 0 && failedFiles == 0 && !m_isCancelled);
  result.cancelled = m_isCancelled.load();
  result.filesConverted = filesConverted;
  result.failedFiles = failedFiles;
  result.errors = errors;
  if (!errors.isEmpty()) {
    result.error = errors.join("\n");
  }

  emit finished(result);
}
