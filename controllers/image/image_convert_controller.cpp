#include "controllers/image/image_convert_controller.h"

#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QIODevice>
#include <QImageReader>
#include <QMarginsF>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QRect>
#include <QRectF>
#include <QSet>
#include <QSize>
#include <QSizeF>
#include <QSvgRenderer>
#include <QtConcurrent/QtConcurrent>
#include <QtGlobal>

namespace {

QString resolveUniqueBaseName(const QString &baseName, const QString &outputDir,
                              const QString &ext,
                              QSet<QString> &usedBaseNames) {
  QString uniqueName = baseName;
  int suffix = 1;
  while (usedBaseNames.contains(uniqueName.toLower()) ||
         QFile::exists(QDir(outputDir).filePath(uniqueName + "." + ext))) {
    uniqueName = QString("%1_%2").arg(baseName).arg(suffix++);
  }
  usedBaseNames.insert(uniqueName.toLower());
  return uniqueName;
}

} // namespace

ImageConvertController::ImageConvertController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<ImageConvertController::Result>(
      "ImageConvertController::Result");
}

QImage ImageConvertController::loadRasterOrSvgImage(const QString &path,
                                                    bool forceOpaqueWhite) {
  const QFileInfo fi(path);
  if (fi.suffix().compare("svg", Qt::CaseInsensitive) == 0) {
    QSvgRenderer renderer(fi.absoluteFilePath());
    if (!renderer.isValid()) {
      return QImage();
    }
    QSize baseSize = renderer.defaultSize();
    if (!baseSize.isValid() || baseSize.width() <= 0 ||
        baseSize.height() <= 0) {
      baseSize = renderer.viewBoxF().size().toSize();
    }
    if (!baseSize.isValid() || baseSize.width() <= 0 ||
        baseSize.height() <= 0) {
      baseSize = QSize(512, 512);
    }
    if (baseSize.width() > 16384 || baseSize.height() > 16384) {
      baseSize.scale(16384, 16384, Qt::KeepAspectRatio);
    }
    baseSize.setWidth(qBound(1, baseSize.width(), 16384));
    baseSize.setHeight(qBound(1, baseSize.height(), 16384));

    QImage img(baseSize, forceOpaqueWhite
                             ? QImage::Format_RGB32
                             : QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) {
      return QImage();
    }
    img.fill(forceOpaqueWhite ? Qt::white : Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter,
                    QRectF(0.0, 0.0, baseSize.width(), baseSize.height()));
    painter.end();
    return img;
  }

  QImageReader reader(fi.absoluteFilePath());
  reader.setAutoTransform(true);
  QImage img = reader.read();
  if (img.isNull()) {
    img = QImage(fi.absoluteFilePath());
  }
  if (!img.isNull() && forceOpaqueWhite && img.hasAlphaChannel()) {
    QImage opaque(img.size(), QImage::Format_RGB32);
    if (!opaque.isNull()) {
      opaque.fill(Qt::white);
      QPainter painter(&opaque);
      painter.drawImage(0, 0, img);
      painter.end();
      return opaque;
    }
  }
  return img;
}

QString
ImageConvertController::extensionToString(DropFileWidget::ImageExtension ext) {
  switch (ext) {
  case DropFileWidget::JPG:
    return "jpg";
  case DropFileWidget::JPEG:
    return "jpeg";
  case DropFileWidget::PNG:
    return "png";
  case DropFileWidget::WEBP:
    return "webp";
  case DropFileWidget::TIFF:
    return "tiff";
  case DropFileWidget::BMP:
    return "bmp";
  case DropFileWidget::GIF:
    return "gif";
  case DropFileWidget::PDF:
    return "pdf";
  default:
    return "";
  }
}

bool ImageConvertController::saveImageToFormat(
    const QImage &image, const QString &outputPath, int quality,
    DropFileWidget::ImageExtension targetExtension) {
  if (image.isNull()) {
    return false;
  }
  const QString format = extensionToString(targetExtension).toLower();
  if (format.isEmpty()) {
    return false;
  }
  const int effectiveQuality =
      (targetExtension == DropFileWidget::PNG) ? -1 : quality;
  QString finalPath = outputPath;
  if (!finalPath.endsWith("." + format, Qt::CaseInsensitive)) {
    finalPath += "." + format;
  }
  return image.save(finalPath, format.toUpper().toUtf8().constData(),
                    effectiveQuality);
}

void ImageConvertController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future =
      QtConcurrent::run([this, params]() { runJob(params); });
  m_watcher.setFuture(future);
}

void ImageConvertController::runJob(const Params &params) {
  Result result;
  switch (params.mode) {
  case Mode::ImagesToPdf:
    result = runImagesToPdf(params);
    break;
  case Mode::SingleImage:
    result = runSingleImage(params);
    break;
  case Mode::BatchImage:
    result = runBatchImage(params);
    break;
  }
  emit finished(result);
}

ImageConvertController::Result
ImageConvertController::runImagesToPdf(const Params &params) {
  Result result;
  result.mode = Mode::ImagesToPdf;
  result.outputPath = params.outputPath;

  const QStringList &sourcePaths = params.sourcePaths;
  const QString &outputPath = params.outputPath;
  const int totalFiles = sourcePaths.size();
  const int progressTotal = (totalFiles == 1) ? 0 : totalFiles;

  if (totalFiles > 0) {
    emit progressChanged(0, progressTotal,
                         QString("Adding %1 to PDF (1 of %2)...")
                             .arg(QFileInfo(sourcePaths.first()).fileName())
                             .arg(totalFiles));
  }

  const QString tempOutputPath = outputPath + ".tmp";
  const int clampedQuality = qBound(1, params.quality, 100);
  const int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
  const qreal scaleFactor = 0.35 + (0.65 * clampedQuality) / 100.0;

  QPdfWriter pdfWriter(tempOutputPath);
  pdfWriter.setResolution(targetDpi);
  pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

  qint64 totalOrigBytes = 0;
  int successCount = 0;
  int skippedCount = 0;
  int startedFiles = 0;
  QStringList includedPaths;
  QString failedPath;
  bool ok = true;

  QPainter painter;
  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled.load()) {
      ok = false;
      break;
    }

    const QString &path = sourcePaths.at(i);
    startedFiles++;
    emit fileStatusChanged(path, static_cast<int>(FileStatus::Processing),
                           "Adding image to PDF");
    emit progressChanged(i, progressTotal,
                         QString("Adding %1 to PDF (%2 of %3)...")
                             .arg(QFileInfo(path).fileName())
                             .arg(i + 1)
                             .arg(totalFiles));

    const QImage origImage = loadRasterOrSvgImage(path, true);
    if (origImage.isNull()) {
      skippedCount++;
      emit fileStatusChanged(path, static_cast<int>(FileStatus::Failed),
                             "Could not load image");
      emit progressChanged(i + 1, progressTotal, QString());
      continue;
    }

    const QSizeF ptSize(origImage.width() * 72.0 / 96.0,
                        origImage.height() * 72.0 / 96.0);
    pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

    QImage processedImg = origImage.convertToFormat(QImage::Format_RGB888);
    if (clampedQuality < 95) {
      const QSize scaledSize(qMax(1, qRound(origImage.width() * scaleFactor)),
                             qMax(1, qRound(origImage.height() * scaleFactor)));
      processedImg = processedImg.scaled(scaledSize, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
      QByteArray jpegBytes;
      QBuffer buffer(&jpegBytes);
      if (buffer.open(QIODevice::WriteOnly)) {
        processedImg.save(&buffer, "JPG", clampedQuality);
        buffer.close();
        QImage reloaded;
        if (reloaded.loadFromData(jpegBytes, "JPG")) {
          processedImg = reloaded;
        }
      }
    }

    if (successCount == 0) {
      if (!painter.begin(&pdfWriter)) {
        ok = false;
        failedPath = path;
        break;
      }
      painter.setRenderHint(QPainter::SmoothPixmapTransform);
      painter.setRenderHint(QPainter::Antialiasing);
    } else {
      if (!pdfWriter.newPage()) {
        ok = false;
        failedPath = path;
        break;
      }
    }

    const QRect pageRect = pdfWriter.pageLayout().paintRectPixels(targetDpi);
    painter.drawImage(pageRect, processedImg);

    totalOrigBytes += QFileInfo(path).size();
    successCount++;
    includedPaths.append(path);
    emit fileStatusChanged(path, static_cast<int>(FileStatus::Waiting),
                           "Added; awaiting PDF save");
    emit progressChanged(i + 1, progressTotal, QString());
  }

  if (ok && !m_isCancelled.load() && successCount > 0) {
    emit progressChanged(startedFiles, progressTotal,
                         QString("Saving %1 (%2 image(s))...")
                             .arg(QFileInfo(outputPath).fileName())
                             .arg(successCount));
  }
  if (painter.isActive() && !painter.end()) {
    ok = false;
  }

  const bool cancelled = m_isCancelled.load();
  bool saved = false;
  if (!ok || cancelled || successCount == 0) {
    QFile::remove(tempOutputPath);
  } else {
    const bool hadExisting = QFile::exists(outputPath);
    const QString backupPath = outputPath + ".bak";
    if (hadExisting) {
      QFile::remove(backupPath);
      if (!QFile::rename(outputPath, backupPath)) {
        QFile::remove(tempOutputPath);
        ok = false;
      }
    }
    if (ok && !QFile::rename(tempOutputPath, outputPath)) {
      QFile::remove(tempOutputPath);
      if (hadExisting) {
        QFile::rename(backupPath, outputPath);
      }
      ok = false;
    }
    if (ok && hadExisting) {
      QFile::remove(backupPath);
    }
    saved = ok;
  }

  for (const QString &path : includedPaths) {
    emit fileStatusChanged(
        path,
        static_cast<int>(
            saved ? FileStatus::Succeeded
                  : (cancelled ? FileStatus::Cancelled : FileStatus::Failed)),
        saved ? QString("Saved in %1").arg(QFileInfo(outputPath).fileName())
              : (cancelled ? "PDF creation cancelled; no output saved"
                           : "Failed to save PDF"));
  }
  if (!failedPath.isEmpty()) {
    emit fileStatusChanged(failedPath, static_cast<int>(FileStatus::Failed),
                           "Failed to add image to PDF");
  }
  for (int i = startedFiles; i < sourcePaths.size(); ++i) {
    emit fileStatusChanged(sourcePaths.at(i),
                           static_cast<int>(cancelled ? FileStatus::Cancelled
                                                      : FileStatus::Failed),
                           cancelled ? "Not processed (cancelled)"
                                     : "Not processed: PDF creation failed");
  }

  result.success = saved;
  result.cancelled = cancelled;
  result.successCount = successCount;
  result.skippedCount = skippedCount;
  result.totalOrigBytes = totalOrigBytes;
  result.totalOutputBytes = saved ? QFileInfo(outputPath).size() : 0;
  return result;
}

ImageConvertController::Result
ImageConvertController::runSingleImage(const Params &params) {
  Result result;
  result.mode = Mode::SingleImage;

  const QString sourcePath =
      params.sourcePaths.isEmpty() ? QString() : params.sourcePaths.first();
  const QString &outputPathWithExt = params.outputPath;
  result.sourcePath = sourcePath;
  result.outputPath = outputPathWithExt;

  const QFileInfo outFi(outputPathWithExt);
  const QString outputPathWithoutExt =
      outFi.dir().filePath(outFi.completeBaseName());

  emit progressChanged(0, 0,
                       QString("Converting %1 (1 of 1)...")
                           .arg(QFileInfo(sourcePath).fileName()));
  emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                         "Converting image");

  const QImage image = loadRasterOrSvgImage(sourcePath, params.forceOpaque);
  FileStatus status = FileStatus::Failed;
  QString detail;
  if (image.isNull()) {
    detail = "Could not load image";
  } else if (m_isCancelled.load()) {
    status = FileStatus::Cancelled;
    detail = "Cancelled before save";
  } else if (saveImageToFormat(image, outputPathWithoutExt, params.quality,
                               params.targetExtension)) {
    status = FileStatus::Succeeded;
    detail =
        QString("Saved as %1").arg(QFileInfo(outputPathWithExt).fileName());
  } else {
    detail = QString("Failed to save image as %1").arg(params.targetFormat);
  }

  emit fileStatusChanged(sourcePath, static_cast<int>(status), detail);
  emit progressChanged(status == FileStatus::Cancelled ? 0 : 1, 1, QString());

  result.success = (status == FileStatus::Succeeded);
  result.cancelled = (status == FileStatus::Cancelled);
  result.successCount = result.success ? 1 : 0;
  result.failedCount = (!result.success && !result.cancelled) ? 1 : 0;
  result.totalOrigBytes = result.success ? QFileInfo(sourcePath).size() : 0;
  result.totalOutputBytes =
      result.success ? QFileInfo(outputPathWithExt).size() : 0;
  result.error = detail;
  return result;
}

ImageConvertController::Result
ImageConvertController::runBatchImage(const Params &params) {
  Result result;
  result.mode = Mode::BatchImage;
  result.outputDir = params.outputDir;

  const QStringList &sourcePaths = params.sourcePaths;
  const QString &outputDir = params.outputDir;
  const int quality = params.quality;
  const DropFileWidget::ImageExtension targetExt = params.targetExtension;
  const QString ext = params.targetFormat.toLower();
  const int totalFiles = sourcePaths.size();

  if (totalFiles > 0) {
    emit progressChanged(0, totalFiles,
                         QString("Converting %1 (1 of %2)...")
                             .arg(QFileInfo(sourcePaths.first()).fileName())
                             .arg(totalFiles));
  }

  int successCount = 0;
  int failureCount = 0;
  int completedFiles = 0;
  qint64 totalOrigBytes = 0;
  qint64 totalOutputBytes = 0;
  QSet<QString> usedBaseNames;

  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled.load()) {
      break;
    }

    const QString &sourcePath = sourcePaths.at(i);
    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Converting image");
    emit progressChanged(completedFiles, totalFiles,
                         QString("Converting %1 (%2 of %3)...")
                             .arg(QFileInfo(sourcePath).fileName())
                             .arg(i + 1)
                             .arg(totalFiles));

    const QImage image = loadRasterOrSvgImage(sourcePath, params.forceOpaque);
    FileStatus status = FileStatus::Failed;
    QString detail = "Could not load image";
    if (image.isNull()) {
      failureCount++;
    } else {
      const QFileInfo fileInfo(sourcePath);
      const QString uniqueBaseName = resolveUniqueBaseName(
          fileInfo.baseName(), outputDir, ext, usedBaseNames);

      if (m_isCancelled.load()) {
        break;
      }

      const QString baseOutputName = QDir(outputDir).filePath(uniqueBaseName);
      if (saveImageToFormat(image, baseOutputName, quality, targetExt)) {
        successCount++;
        totalOrigBytes += fileInfo.size();
        const QString outputFilePath = baseOutputName + "." + ext;
        totalOutputBytes += QFileInfo(outputFilePath).size();
        status = FileStatus::Succeeded;
        detail = QString("Saved as %1.%2").arg(uniqueBaseName).arg(ext);
      } else {
        failureCount++;
        detail = QString("Failed to save as %1").arg(ext.toUpper());
      }
    }

    completedFiles++;
    emit fileStatusChanged(sourcePath, static_cast<int>(status), detail);
    emit progressChanged(completedFiles, totalFiles, QString());
  }

  for (int i = completedFiles; i < sourcePaths.size(); ++i) {
    emit fileStatusChanged(sourcePaths.at(i),
                           static_cast<int>(FileStatus::Cancelled),
                           "Not processed (cancelled)");
  }

  result.cancelled = m_isCancelled.load() && completedFiles < totalFiles;
  result.success = !result.cancelled && successCount > 0;
  result.successCount = successCount;
  result.failedCount = failureCount;
  result.totalOrigBytes = totalOrigBytes;
  result.totalOutputBytes = totalOutputBytes;
  return result;
}
