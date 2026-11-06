#include "controllers/pdf/merge_controller.h"
#include "controllers/pdf/pdf_utils.h"

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

MergeController::MergeController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<MergeController::Result>();
}

void MergeController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

bool MergeController::mergePdfsWithGhostscript(const QString &executable,
                                               const QStringList &inputPaths,
                                               const QString &tempOutputPath,
                                               QString &error) {
  QStringList args = {"-sDEVICE=pdfwrite",
                      "-dCompatibilityLevel=1.7",
                      "-dSAFER",
                      "-dPDFSTOPONERROR",
                      "-dNOPAUSE",
                      "-dQUIET",
                      "-dBATCH",
                      "-dAutoRotatePages=/None",
                      "-dCompressFonts=true",
                      "-dCompressStreams=true",
                      "-dSubsetFonts=true",
                      "-dPreserveAnnots=true",
                      "-sOutputFile=candidate.pdf",
                      "-f"};
  for (const QString &path : inputPaths) {
    args << QDir::toNativeSeparators(path);
  }
  return pdfutil::runGhostscriptProcess(
      executable, args, QFileInfo(tempOutputPath).absolutePath(),
      tempOutputPath, m_isCancelled, error, false);
}

bool MergeController::mergePdfsWithQtPdf(const QStringList &inputPaths,
                                         const QString &tempOutputPath,
                                         QString &error) {
  QPdfWriter writer(tempOutputPath);
  writer.setResolution(300);
  std::unique_ptr<QPainter> painter;

  for (const QString &inputPath : inputPaths) {
    if (m_isCancelled) {
      return false;
    }
    QPdfDocument doc;
    const auto loadErr = doc.load(inputPath);
    if (loadErr != QPdfDocument::Error::None || doc.pageCount() <= 0) {
      error = QString("Cannot open '%1' for merging.")
                  .arg(QFileInfo(inputPath).fileName());
      return false;
    }

    for (int page = 0; page < doc.pageCount(); ++page) {
      if (m_isCancelled) {
        return false;
      }
      const QSizeF ptSize = doc.pagePointSize(page);
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

      const QSize renderSize(qMax(1, qRound(ptSize.width() * 300.0 / 72.0)),
                             qMax(1, qRound(ptSize.height() * 300.0 / 72.0)));
      const QImage pageImg = pdfutil::renderPdfPageOnWhite(doc, page, renderSize);
      if (pageImg.isNull()) {
        error = QString("Failed to render page %1 of '%2'.")
                    .arg(page + 1)
                    .arg(QFileInfo(inputPath).fileName());
        return false;
      }
      painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
      painter->drawImage(
          QRectF(0, 0, painter->device()->width(), painter->device()->height()),
          pageImg);
    }
  }

  if (painter) {
    painter->end();
  }
  return QFileInfo(tempOutputPath).size() > 0;
}

void MergeController::runJob(const Params &params) {
  Result result;
  result.outputPath = params.outputPath;

  const QStringList &orderedPaths = params.inputPaths;
  emit progressChanged(0, orderedPaths.size(),
                       QString("Merging %1 PDF files...").arg(orderedPaths.size()));

  for (const QString &srcPath : orderedPaths) {
    emit fileStatusChanged(srcPath, static_cast<int>(FileStatus::Processing),
                           "Merging into combined PDF");
  }

  QString error;
  int expectedTotalPages = 0;
  for (const QString &srcPath : orderedPaths) {
    if (m_isCancelled) {
      break;
    }
    const QFileInfo srcInfo(srcPath);
    if (!srcInfo.isFile() || srcInfo.size() <= 0) {
      error = QString("Input file '%1' is missing or empty.")
                  .arg(srcInfo.fileName());
      break;
    }
    QPdfDocument doc;
    const auto err = doc.load(srcInfo.absoluteFilePath());
    if (err != QPdfDocument::Error::None || doc.pageCount() <= 0) {
      error = QString("Cannot open '%1' (damaged or password-protected).")
                  .arg(srcInfo.fileName());
      break;
    }
    expectedTotalPages += doc.pageCount();
  }

  bool succeeded = false;
  qint64 outputBytes = 0;
  const QFileInfo destinationInfo(params.outputPath);

  if (error.isEmpty() && !m_isCancelled) {
    QTemporaryDir tempDir(
        destinationInfo.absoluteDir().filePath(".pdf-merge-XXXXXX"));
    if (!tempDir.isValid()) {
      error = QString("Cannot create temporary directory: %1")
                  .arg(tempDir.errorString());
    } else {
      const QString candidatePath = tempDir.filePath("candidate.pdf");
      const QString gsExec = pdfutil::findGhostscriptExecutable();
      bool generated = false;
      if (!gsExec.isEmpty()) {
        QString gsErr;
        generated = mergePdfsWithGhostscript(gsExec, orderedPaths,
                                             candidatePath, gsErr);
        if (generated) {
          QPdfDocument checkDoc;
          if (checkDoc.load(candidatePath) != QPdfDocument::Error::None ||
              checkDoc.pageCount() != expectedTotalPages) {
            generated = false;
            QFile::remove(candidatePath);
          }
        }
      }
      if (!generated && !m_isCancelled) {
        generated = mergePdfsWithQtPdf(orderedPaths, candidatePath, error);
      }

      if (generated && !m_isCancelled) {
        QPdfDocument candidateDoc;
        if (candidateDoc.load(candidatePath) != QPdfDocument::Error::None ||
            candidateDoc.pageCount() != expectedTotalPages) {
          error = "Merged PDF validation failed (unexpected page count).";
        } else {
          candidateDoc.close();
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

  for (const QString &srcPath : orderedPaths) {
    if (m_isCancelled) {
      emit fileStatusChanged(srcPath, static_cast<int>(FileStatus::Cancelled),
                             "Merge cancelled");
    } else if (succeeded) {
      emit fileStatusChanged(srcPath, static_cast<int>(FileStatus::Succeeded),
                             QString("Merged into %1").arg(params.outputPath));
    } else {
      emit fileStatusChanged(srcPath, static_cast<int>(FileStatus::Failed),
                             error);
    }
  }

  if (!m_isCancelled && succeeded) {
    emit progressChanged(orderedPaths.size(), orderedPaths.size(), "Completed");
  }

  result.success = succeeded;
  result.cancelled = m_isCancelled.load();
  result.totalPages = expectedTotalPages;
  result.outputBytes = outputBytes;
  result.error = error;

  emit finished(result);
}
