#include "controllers/split_controller.h"
#include "controllers/pdf_utils.h"

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

SplitController::SplitController(QObject *parent)
    : PdfToolController(parent) {
  qRegisterMetaType<SplitController::Result>();
}

void SplitController::start(const Params &params) {
  waitForRunningJob();
  m_isCancelled = false;
  QFuture<void> future = QtConcurrent::run([this, params]() {
    runJob(params);
  });
  m_watcher.setFuture(future);
}

bool SplitController::extractPageRangeWithGhostscript(
    const QString &executable, const QString &inputPath, int firstPage,
    int lastPage, const QString &tempOutputPath, QString &error) {
  QStringList args = {"-sDEVICE=pdfwrite",
                      "-dCompatibilityLevel=1.7",
                      "-dSAFER",
                      "-dPDFSTOPONERROR",
                      "-dNOPAUSE",
                      "-dQUIET",
                      "-dBATCH",
                      "-dAutoRotatePages=/None",
                      QString("-dFirstPage=%1").arg(firstPage),
                      QString("-dLastPage=%1").arg(lastPage),
                      "-sOutputFile=candidate.pdf",
                      "-f",
                      QDir::toNativeSeparators(inputPath)};
  return pdfutil::runGhostscriptProcess(
      executable, args, QFileInfo(tempOutputPath).absolutePath(),
      tempOutputPath, m_isCancelled, error, false);
}

bool SplitController::extractPageRangeWithQtPdf(QPdfDocument &sourceDoc,
                                               int firstPage, int lastPage,
                                               const QString &tempOutputPath,
                                               QString &error) {
  QPdfWriter writer(tempOutputPath);
  writer.setResolution(300);
  std::unique_ptr<QPainter> painter;

  for (int page = firstPage - 1; page <= lastPage - 1; ++page) {
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

    const QSize renderSize(qMax(1, qRound(ptSize.width() * 300.0 / 72.0)),
                           qMax(1, qRound(ptSize.height() * 300.0 / 72.0)));
    const QImage pageImg = pdfutil::renderPdfPageOnWhite(sourceDoc, page, renderSize);
    if (pageImg.isNull()) {
      error = QString("Failed to render page %1.").arg(page + 1);
      return false;
    }
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(
        QRectF(0, 0, painter->device()->width(), painter->device()->height()),
        pageImg);
  }

  if (painter) {
    painter->end();
  }
  return QFileInfo(tempOutputPath).size() > 0;
}

void SplitController::runJob(const Params &params) {
  Result result;
  result.outputDir = params.outputDir;

  QStringList sourcePaths = params.sourcePaths;
  if (sourcePaths.isEmpty() && !params.sourcePath.isEmpty()) {
    sourcePaths.append(params.sourcePath);
  }

  emit progressChanged(0, sourcePaths.size(), "Splitting PDF files...");

  int totalFilesCreated = 0;
  int failedDocs = 0;
  QStringList errors;
  QSet<QString> usedFileNames;
  const QString gsExec = pdfutil::findGhostscriptExecutable();

  for (int i = 0; i < sourcePaths.size(); ++i) {
    if (m_isCancelled) {
      break;
    }
    const QString &sourcePath = sourcePaths.at(i);
    const QFileInfo sourceInfo(sourcePath);

    emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Processing),
                           "Splitting PDF pages");
    emit progressChanged(
        i, sourcePaths.size(),
        QString("Splitting %1 (%2 of %3)...")
            .arg(sourceInfo.fileName())
            .arg(i + 1)
            .arg(sourcePaths.size()));

    QPdfDocument sourceDoc;
    const auto loadErr = sourceDoc.load(sourceInfo.absoluteFilePath());
    if (loadErr != QPdfDocument::Error::None || sourceDoc.pageCount() <= 0) {
      failedDocs++;
      const QString err = "Cannot open PDF (damaged or encrypted).";
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed), err);
      continue;
    }

    const int pageCount = sourceDoc.pageCount();
    QVector<QPair<int, int>> ranges;
    QString rangeErr;
    if (params.mode == 0) {
      if (!pdfutil::parsePageRanges(params.rangeSpec, pageCount, ranges, rangeErr)) {
        failedDocs++;
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), rangeErr));
        emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed), rangeErr);
        continue;
      }
    } else if (params.mode == 1) {
      for (int start = 1; start <= pageCount; start += params.interval) {
        const int end = qMin(pageCount, start + params.interval - 1);
        ranges.append(qMakePair(start, end));
      }
    } else {
      for (int p = 1; p <= pageCount; ++p) {
        ranges.append(qMakePair(p, p));
      }
    }

    int createdForThisPdf = 0;
    QString docError;
    for (const auto &range : ranges) {
      if (m_isCancelled) {
        break;
      }
      const int firstPage = range.first;
      const int lastPage = range.second;
      const int expectedPages = lastPage - firstPage + 1;

      const QString baseStem = (firstPage == lastPage)
                                   ? QString("%1_page_%2")
                                         .arg(sourceInfo.baseName())
                                         .arg(firstPage)
                                   : QString("%1_pages_%2-%3")
                                         .arg(sourceInfo.baseName())
                                         .arg(firstPage)
                                         .arg(lastPage);

      QString uniqueStem = baseStem;
      int suffix = 1;
      while (usedFileNames.contains(uniqueStem.toLower()) ||
             QFile::exists(QDir(params.outputDir).filePath(uniqueStem + ".pdf"))) {
        uniqueStem = QString("%1_%2").arg(baseStem).arg(suffix++);
      }
      usedFileNames.insert(uniqueStem.toLower());
      const QString outFilePath =
          QDir(params.outputDir).filePath(uniqueStem + ".pdf");

      QTemporaryDir tempDir(QDir(params.outputDir).filePath(".pdf-split-XXXXXX"));
      if (!tempDir.isValid()) {
        docError = tempDir.errorString();
        break;
      }
      const QString candidatePath = tempDir.filePath("candidate.pdf");
      bool extracted = false;
      if (!gsExec.isEmpty()) {
        QString gsErr;
        extracted = extractPageRangeWithGhostscript(
            gsExec, sourceInfo.absoluteFilePath(), firstPage, lastPage,
            candidatePath, gsErr);
        if (extracted) {
          QPdfDocument checkDoc;
          if (checkDoc.load(candidatePath) != QPdfDocument::Error::None ||
              checkDoc.pageCount() != expectedPages) {
            extracted = false;
            QFile::remove(candidatePath);
          }
        }
      }
      if (!extracted && !m_isCancelled) {
        extracted = extractPageRangeWithQtPdf(sourceDoc, firstPage, lastPage,
                                              candidatePath, docError);
      }
      if (!extracted || m_isCancelled) {
        if (docError.isEmpty()) {
          docError = "Failed to extract page range.";
        }
        break;
      }

      QPdfDocument verifyDoc;
      if (verifyDoc.load(candidatePath) != QPdfDocument::Error::None ||
          verifyDoc.pageCount() != expectedPages) {
        docError = "Extracted PDF failed validation.";
        break;
      }
      verifyDoc.close();

      if (!pdfutil::savePdfAtomically(candidatePath, outFilePath, m_isCancelled,
                                      docError)) {
        break;
      }
      createdForThisPdf++;
      totalFilesCreated++;
      result.generatedFiles.append(outFilePath);
    }

    sourceDoc.close();

    if (m_isCancelled) {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Cancelled),
                             "Split cancelled");
    } else if (docError.isEmpty() && createdForThisPdf > 0) {
      emit fileStatusChanged(
          sourcePath, static_cast<int>(FileStatus::Succeeded),
          QString("Created %1 PDF file(s) in %2")
              .arg(createdForThisPdf)
              .arg(params.outputDir));
    } else {
      emit fileStatusChanged(sourcePath, static_cast<int>(FileStatus::Failed),
                             docError);
    }

    if (!m_isCancelled) {
      emit progressChanged(i + 1, sourcePaths.size(),
                           QString("Processed %1 of %2").arg(i + 1).arg(sourcePaths.size()));
    }

    if (!docError.isEmpty() && !m_isCancelled) {
      failedDocs++;
      errors.append(QString("%1: %2").arg(sourceInfo.fileName(), docError));
    }
  }

  result.success = (totalFilesCreated > 0 && failedDocs == 0);
  result.cancelled = m_isCancelled.load();
  result.createdCount = totalFilesCreated;
  result.failedDocs = failedDocs;
  result.errors = errors;
  if (!errors.isEmpty()) {
    result.error = errors.join("\n");
  }

  emit finished(result);
}

