#include "controllers/image/ico_export_controller.h"
#include "controllers/pdf/pdf_utils.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QRectF>
#include <QSaveFile>
#include <QSet>
#include <QSvgRenderer>
#include <QtConcurrent/QtConcurrent>
#include <QtEndian>
#include <algorithm>
#include <cstdint>

namespace {

void appendUInt16LE(QByteArray &out, uint16_t val) {
  const uint16_t le = qToLittleEndian<uint16_t>(val);
  out.append(reinterpret_cast<const char *>(&le), sizeof(uint16_t));
}

void appendUInt32LE(QByteArray &out, uint32_t val) {
  const uint32_t le = qToLittleEndian<uint32_t>(val);
  out.append(reinterpret_cast<const char *>(&le), sizeof(uint32_t));
}

} // namespace

IcoExportController::IcoExportController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<IcoExportController::Result>();
}

void IcoExportController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

void IcoExportController::runJob(const Params &params) {
  Result result;
  result.outputDir = params.outputDir;

  QVector<int> validSizes;
  for (int sz : params.sizes) {
    if (sz >= 1 && sz <= 256 && !validSizes.contains(sz)) {
      validSizes.append(sz);
    }
  }
  if (validSizes.isEmpty()) {
    validSizes = {16, 32, 48, 64, 128, 256};
  }
  std::sort(validSizes.begin(), validSizes.end());
  result.sizes = validSizes;

  const QStringList &sourcePaths = params.sourcePaths;
  const int totalFiles = sourcePaths.size();

  emit progressChanged(0, totalFiles, "Building multi-size .ico icons...");

  int iconsCreated = 0;
  int failedFiles = 0;
  QStringList errors;
  QSet<QString> usedFileNames;

  for (int i = 0; i < totalFiles; ++i) {
    if (m_isCancelled) {
      for (int rem = i; rem < totalFiles; ++rem) {
        emit fileStatusChanged(sourcePaths.at(rem),
                               static_cast<int>(FileStatus::Cancelled),
                               "Icon export cancelled");
      }
      break;
    }

    const QString &sourcePath = sourcePaths.at(i);
    const QFileInfo sourceInfo(sourcePath);

    emit fileStatusChanged(
        sourcePath, static_cast<int>(FileStatus::Processing),
        QString("Packing %1 icon size(s) into .ico").arg(validSizes.size()));
    emit progressChanged(
        i, totalFiles,
        QString("Creating icon for %1 (%2 of %3)...")
            .arg(sourceInfo.fileName())
            .arg(i + 1)
            .arg(totalFiles));

    const bool isSvg =
        (sourceInfo.suffix().compare("svg", Qt::CaseInsensitive) == 0);
    QSvgRenderer svgRenderer;
    QImage srcImage;
    QSize svgBaseSize(256, 256);

    if (isSvg) {
      if (!svgRenderer.load(sourceInfo.absoluteFilePath()) ||
          !svgRenderer.isValid()) {
        failedFiles++;
        const QString err = "Invalid or unreadable SVG file.";
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
        emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                               err);
        continue;
      }
      svgBaseSize = svgRenderer.defaultSize();
      if (!svgBaseSize.isValid() || svgBaseSize.width() <= 0 ||
          svgBaseSize.height() <= 0) {
        svgBaseSize = svgRenderer.viewBoxF().size().toSize();
      }
      if (!svgBaseSize.isValid() || svgBaseSize.width() <= 0 ||
          svgBaseSize.height() <= 0) {
        svgBaseSize = QSize(256, 256);
      }
    } else {
      QImageReader reader(sourceInfo.absoluteFilePath());
      reader.setAutoTransform(true);
      srcImage = reader.read();
      if (srcImage.isNull()) {
        failedFiles++;
        const QString err = "Cannot load source image.";
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
        emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                               err);
        continue;
      }
    }

    struct EncodedFrame {
      int size{0};
      QByteArray pngBytes;
    };
    QVector<EncodedFrame> frames;
    frames.reserve(validSizes.size());
    QString frameError;

    for (int sz : validSizes) {
      if (m_isCancelled) {
        break;
      }

      QImage frame(sz, sz, QImage::Format_ARGB32);
      if (frame.isNull()) {
        frameError = QString("Failed to allocate %1×%1 icon frame.").arg(sz);
        break;
      }
      frame.fill(Qt::transparent);

      if (isSvg) {
        const qreal scale = qMin(static_cast<qreal>(sz) / svgBaseSize.width(),
                                 static_cast<qreal>(sz) / svgBaseSize.height());
        const qreal drawW = svgBaseSize.width() * scale;
        const qreal drawH = svgBaseSize.height() * scale;
        const QRectF targetRect((sz - drawW) / 2.0, (sz - drawH) / 2.0, drawW,
                                drawH);
        QPainter painter(&frame);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        svgRenderer.render(&painter, targetRect);
      } else {
        const QImage scaled =
            srcImage.scaled(sz, sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (scaled.isNull()) {
          frameError = QString("Failed to scale image to %1×%1.").arg(sz);
          break;
        }
        QPainter painter(&frame);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const int offsetX = (sz - scaled.width()) / 2;
        const int offsetY = (sz - scaled.height()) / 2;
        painter.drawImage(offsetX, offsetY, scaled);
      }

      QByteArray pngData;
      QBuffer buf(&pngData);
      if (!buf.open(QIODevice::WriteOnly) || !frame.save(&buf, "PNG", -1)) {
        frameError = QString("Failed to encode %1×%1 PNG payload.").arg(sz);
        break;
      }
      buf.close();
      frames.push_back({sz, pngData});
    }

    if (m_isCancelled) {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Cancelled),
                             "Icon export cancelled");
      for (int rem = i + 1; rem < totalFiles; ++rem) {
        emit fileStatusChanged(sourcePaths.at(rem),
                               static_cast<int>(FileStatus::Cancelled),
                               "Icon export cancelled");
      }
      break;
    }

    if (!frameError.isEmpty()) {
      failedFiles++;
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), frameError));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             frameError);
      continue;
    }

    const QString baseStem = sourceInfo.completeBaseName().isEmpty()
                                 ? QStringLiteral("icon")
                                 : sourceInfo.completeBaseName();
    QString uniqueStem = baseStem;
    int suffix = 1;
    while (true) {
      const QString candidateName = uniqueStem + ".ico";
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
    const QString outFileName = uniqueStem + ".ico";
    usedFileNames.insert(outFileName.toLower());
    const QString outIcoPath = QDir(params.outputDir).filePath(outFileName);

    QByteArray icoHeader;
    icoHeader.reserve(6 + 16 * frames.size());
    appendUInt16LE(icoHeader, 0); // idReserved = 0
    appendUInt16LE(icoHeader, 1); // idType = 1 (ICO)
    appendUInt16LE(icoHeader, static_cast<uint16_t>(frames.size()));

    uint32_t offset = static_cast<uint32_t>(6 + 16 * frames.size());
    for (const EncodedFrame &ef : frames) {
      const uint8_t bWidth = (ef.size >= 256) ? 0 : static_cast<uint8_t>(ef.size);
      const uint8_t bHeight = (ef.size >= 256) ? 0 : static_cast<uint8_t>(ef.size);
      icoHeader.append(static_cast<char>(bWidth));
      icoHeader.append(static_cast<char>(bHeight));
      icoHeader.append(static_cast<char>(0)); // bColorCount
      icoHeader.append(static_cast<char>(0)); // bReserved
      appendUInt16LE(icoHeader, 1);           // wPlanes = 1
      appendUInt16LE(icoHeader, 32);          // wBitCount = 32
      const uint32_t bytesInRes = static_cast<uint32_t>(ef.pngBytes.size());
      appendUInt32LE(icoHeader, bytesInRes);
      appendUInt32LE(icoHeader, offset);
      offset += bytesInRes;
    }

    QSaveFile saveFile(outIcoPath);
    saveFile.setDirectWriteFallback(false);
    bool writeOk = saveFile.open(QIODevice::WriteOnly);
    if (writeOk) {
      writeOk = (saveFile.write(icoHeader) == icoHeader.size());
    }
    if (writeOk) {
      for (const EncodedFrame &ef : frames) {
        if (saveFile.write(ef.pngBytes) != ef.pngBytes.size()) {
          writeOk = false;
          break;
        }
      }
    }
    if (writeOk) {
      writeOk = saveFile.commit();
    }

    if (!writeOk) {
      failedFiles++;
      const QString err = QString("Failed to write '%1'.").arg(outFileName);
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             err);
      continue;
    }

    iconsCreated++;
    emit fileStatusChanged(
        sourcePath, static_cast<int>(FileStatus::Succeeded),
        QString("Saved %1 (%2 resolution(s))")
            .arg(outFileName)
            .arg(frames.size()));
    emit progressChanged(
        i + 1, totalFiles,
        QString("Created %1 of %2 icon(s)").arg(i + 1).arg(totalFiles));
  }

  result.success = (iconsCreated > 0 && failedFiles == 0 && !m_isCancelled);
  result.cancelled = m_isCancelled.load();
  result.iconsCreated = iconsCreated;
  result.failedFiles = failedFiles;
  result.errors = errors;
  if (!errors.isEmpty()) {
    result.error = errors.join("\n");
  }

  emit finished(result);
}
