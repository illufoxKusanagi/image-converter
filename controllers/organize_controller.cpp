#include "controllers/organize_controller.h"
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
#include <QTransform>
#include <QtConcurrent/QtConcurrent>
#include <memory>

OrganizeController::OrganizeController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<OrganizeController::PageOp>();
  qRegisterMetaType<OrganizeController::Result>();
}

void OrganizeController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

void OrganizeController::runJob(const Params &params) {
  Result result;
  result.outputPath = params.outputPath;
  result.pageCount = params.pageOps.size();

  emit progressChanged(0, params.pageOps.size(),
                       QString("Saving %1 organized page(s)...").arg(params.pageOps.size()));
  emit fileStatusChanged(params.sourcePath,
                         static_cast<int>(FileStatus::Processing),
                         "Writing organized PDF");

  QString error;
  bool succeeded = false;
  qint64 outputBytes = 0;
  const QFileInfo destinationInfo(params.outputPath);

  QPdfDocument sourceDoc;
  const auto loadErr = sourceDoc.load(params.sourcePath);
  if (loadErr != QPdfDocument::Error::None || sourceDoc.pageCount() <= 0) {
    error = "Cannot open source PDF.";
  } else {
    QTemporaryDir tempDir(
        destinationInfo.absoluteDir().filePath(".pdf-organize-XXXXXX"));
    if (!tempDir.isValid()) {
      error = tempDir.errorString();
    } else {
      const QString candidatePath = tempDir.filePath("candidate.pdf");
      QPdfWriter writer(candidatePath);
      writer.setResolution(300);
      std::unique_ptr<QPainter> painter;

      for (int i = 0; i < params.pageOps.size(); ++i) {
        if (m_isCancelled) {
          break;
        }
        const PageOp &op = params.pageOps.at(i);
        if (op.originalPageIndex < 0 ||
            op.originalPageIndex >= sourceDoc.pageCount()) {
          error = "Page index out of bounds.";
          break;
        }

        const QSizeF origPtSize =
            sourceDoc.pagePointSize(op.originalPageIndex);
        const bool swapDims =
            (op.rotationDegrees == 90 || op.rotationDegrees == 270);
        const QSizeF effectivePtSize =
            swapDims ? QSizeF(origPtSize.height(), origPtSize.width())
                     : origPtSize;

        QPageLayout layout(QPageSize(effectivePtSize, QPageSize::Point),
                           QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                           QPageLayout::Point);
        writer.setPageLayout(layout);

        if (!painter) {
          painter = std::make_unique<QPainter>(&writer);
          if (!painter->isActive()) {
            error = "Failed to initialize PDF writer.";
            break;
          }
        } else {
          writer.newPage();
        }

        const QSize renderSize(
            qMax(1, qRound(origPtSize.width() * 300.0 / 72.0)),
            qMax(1, qRound(origPtSize.height() * 300.0 / 72.0)));
        QImage pageImg =
            pdfutil::renderPdfPageOnWhite(sourceDoc, op.originalPageIndex, renderSize);
        if (pageImg.isNull()) {
          error = QString("Failed to render page %1.")
                      .arg(op.originalPageIndex + 1);
          break;
        }
        if (op.rotationDegrees != 0) {
          pageImg = pageImg.transformed(QTransform().rotate(op.rotationDegrees),
                                        Qt::SmoothTransformation);
        }

        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawImage(QRectF(0, 0, painter->device()->width(),
                                  painter->device()->height()),
                           pageImg);

        if (!m_isCancelled) {
          emit progressChanged(
              i + 1, params.pageOps.size(),
              QString("Rendered page %1 of %2...").arg(i + 1).arg(params.pageOps.size()));
        }
      }

      if (painter) {
        painter->end();
      }

      if (error.isEmpty() && !m_isCancelled) {
        QPdfDocument verifyDoc;
        if (verifyDoc.load(candidatePath) != QPdfDocument::Error::None ||
            verifyDoc.pageCount() != params.pageOps.size()) {
          error = "Organized PDF failed validation.";
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
  }

  sourceDoc.close();

  if (m_isCancelled) {
    emit fileStatusChanged(params.sourcePath,
                           static_cast<int>(FileStatus::Cancelled),
                           "Operation cancelled");
  } else if (succeeded) {
    emit fileStatusChanged(
        params.sourcePath, static_cast<int>(FileStatus::Succeeded),
        QString("Saved organized PDF to %1").arg(params.outputPath));
  } else {
    emit fileStatusChanged(params.sourcePath,
                           static_cast<int>(FileStatus::Failed), error);
  }

  result.success = succeeded;
  result.cancelled = m_isCancelled.load();
  result.outputBytes = outputBytes;
  result.error = error;

  emit finished(result);
}

