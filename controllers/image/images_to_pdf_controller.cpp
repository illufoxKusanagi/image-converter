#include "controllers/images_to_pdf_controller.h"
#include "controllers/pdf_utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QtConcurrent/QtConcurrent>
#include <memory>

ImagesToPdfController::ImagesToPdfController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<ImagesToPdfController::Result>();
}

void ImagesToPdfController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

void ImagesToPdfController::runJob(const Params &params) {
  Result result;
  result.outputPath = params.outputPath;
  result.pageCount = params.imagePaths.size();

  const QStringList &orderedImages = params.imagePaths;
  const QVector<qreal> marginPts = {0.0, 18.0, 36.0, 54.0};
  const qreal marginPt = marginPts.value(params.marginMode, 0.0);

  emit progressChanged(
      0, orderedImages.size(),
      QString("Combining %1 image(s) into PDF...").arg(orderedImages.size()));

  for (const QString &imgPath : orderedImages) {
    emit fileStatusChanged(imgPath, static_cast<int>(FileStatus::Processing),
                           "Adding image to PDF");
  }

  QString error;
  bool succeeded = false;
  qint64 outputBytes = 0;
  const QFileInfo destinationInfo(params.outputPath);

  QTemporaryDir tempDir(
      destinationInfo.absoluteDir().filePath(".img-to-pdf-XXXXXX"));
  if (!tempDir.isValid()) {
    error = tempDir.errorString();
  } else {
    const QString candidatePath = tempDir.filePath("candidate.pdf");
    QPdfWriter writer(candidatePath);
    writer.setResolution(300);
    std::unique_ptr<QPainter> painter;

    for (int i = 0; i < orderedImages.size(); ++i) {
      if (m_isCancelled) {
        break;
      }
      const QString &imgPath = orderedImages.at(i);
      QImage rawImg(imgPath);
      if (rawImg.isNull()) {
        error = QString("Cannot load image '%1'.")
                    .arg(QFileInfo(imgPath).fileName());
        break;
      }

      // Composite onto opaque white so alpha channels render cleanly on white paper
      QImage img(rawImg.size(), QImage::Format_RGB32);
      img.fill(Qt::white);
      {
        QPainter imgPainter(&img);
        imgPainter.drawImage(0, 0, rawImg);
      }

      if (params.pageSizeMode == 0) {
        // Fit to Image
        double dpiX = rawImg.dotsPerMeterX() > 0
                          ? rawImg.dotsPerMeterX() * 0.0254
                          : 96.0;
        double dpiY = rawImg.dotsPerMeterY() > 0
                          ? rawImg.dotsPerMeterY() * 0.0254
                          : 96.0;
        if (dpiX < 36.0 || dpiX > 1200.0)
          dpiX = 96.0;
        if (dpiY < 36.0 || dpiY > 1200.0)
          dpiY = 96.0;
        const qreal wPt =
            qMax(72.0, img.width() * 72.0 / dpiX) + 2.0 * marginPt;
        const qreal hPt =
            qMax(72.0, img.height() * 72.0 / dpiY) + 2.0 * marginPt;
        QPageLayout layout(QPageSize(QSizeF(wPt, hPt), QPageSize::Point),
                           QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                           QPageLayout::Point);
        writer.setPageLayout(layout);
      } else {
        const QPageSize pageSize(params.pageSizeMode == 1 ? QPageSize::A4
                                                         : QPageSize::Letter);
        QPageLayout::Orientation orient = QPageLayout::Portrait;
        if (params.orientationMode == 0) {
          orient = (img.width() > img.height()) ? QPageLayout::Landscape
                                                : QPageLayout::Portrait;
        } else if (params.orientationMode == 2) {
          orient = QPageLayout::Landscape;
        }
        QPageLayout layout(pageSize, orient, QMarginsF(0, 0, 0, 0),
                           QPageLayout::Point);
        writer.setPageLayout(layout);
      }

      if (!painter) {
        painter = std::make_unique<QPainter>(&writer);
        if (!painter->isActive()) {
          error = "Failed to initialize PDF writer.";
          break;
        }
      } else {
        writer.newPage();
      }

      const qreal devW = painter->device()->width();
      const qreal devH = painter->device()->height();
      painter->fillRect(QRectF(0, 0, devW, devH), Qt::white);

      const qreal marginPx = marginPt * 300.0 / 72.0;
      const QRectF availRect(marginPx, marginPx,
                             qMax(1.0, devW - 2.0 * marginPx),
                             qMax(1.0, devH - 2.0 * marginPx));
      const qreal scale = qMin(availRect.width() / img.width(),
                               availRect.height() / img.height());
      const qreal drawW = img.width() * scale;
      const qreal drawH = img.height() * scale;
      const QRectF targetRect(
          availRect.left() + (availRect.width() - drawW) / 2.0,
          availRect.top() + (availRect.height() - drawH) / 2.0, drawW, drawH);

      painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
      painter->drawImage(targetRect, img);

      if (!m_isCancelled) {
        emit progressChanged(
            i + 1, orderedImages.size(),
            QString("Added %1 of %2 image(s)...").arg(i + 1).arg(orderedImages.size()));
      }
    }

    if (painter) {
      painter->end();
    }

    if (error.isEmpty() && !m_isCancelled) {
      QPdfDocument verifyDoc;
      if (verifyDoc.load(candidatePath) != QPdfDocument::Error::None ||
          verifyDoc.pageCount() != orderedImages.size()) {
        error = "Generated PDF failed validation.";
      } else {
        verifyDoc.close();
        if (pdfutil::savePdfAtomically(candidatePath,
                                       destinationInfo.absoluteFilePath(),
                                       m_isCancelled, error)) {
          succeeded = true;
          outputBytes = QFileInfo(destinationInfo.absoluteFilePath()).size();
        }
      }
    }
  }

  for (const QString &imgPath : orderedImages) {
    if (m_isCancelled) {
      emit fileStatusChanged(imgPath, static_cast<int>(FileStatus::Cancelled),
                             "Creation cancelled");
    } else if (succeeded) {
      emit fileStatusChanged(imgPath, static_cast<int>(FileStatus::Succeeded),
                             QString("Added to %1").arg(params.outputPath));
    } else {
      emit fileStatusChanged(imgPath, static_cast<int>(FileStatus::Failed),
                             error);
    }
  }

  result.success = succeeded;
  result.cancelled = m_isCancelled.load();
  result.outputBytes = outputBytes;
  result.error = error;

  emit finished(result);
}

