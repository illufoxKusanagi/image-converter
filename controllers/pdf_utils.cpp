#include "controllers/pdf_utils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QPainter>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QStandardPaths>

namespace pdfutil {

bool isSamePdf(const QFileInfo &source, const QFileInfo &destination) {
  const QString canonicalSource = source.canonicalFilePath();
  const QString canonicalDestination = destination.canonicalFilePath();
#ifdef Q_OS_WIN
  constexpr Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
  constexpr Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
  return source == destination ||
         (!canonicalSource.isEmpty() && !canonicalDestination.isEmpty() &&
          canonicalSource.compare(canonicalDestination, pathCase) == 0);
}

bool savePdfAtomically(const QString &candidatePath, const QString &outputPath,
                       const std::atomic<bool> &cancelled, QString &error) {
  QFile candidate(candidatePath);
  QSaveFile destination(outputPath);
  // Never fall back to truncating an existing destination on a failed save.
  destination.setDirectWriteFallback(false);
  if (!candidate.open(QIODevice::ReadOnly)) {
    error = QString("Cannot read the generated PDF: %1")
                .arg(candidate.errorString());
    return false;
  }
  if (!destination.open(QIODevice::WriteOnly)) {
    error = QString("Cannot save the output PDF: %1")
                .arg(destination.errorString());
    return false;
  }

  while (!candidate.atEnd()) {
    if (cancelled) {
      return false;
    }
    const QByteArray chunk = candidate.read(1024 * 1024);
    if (candidate.error() != QFileDevice::NoError) {
      error = QString("Cannot read the generated PDF: %1")
                  .arg(candidate.errorString());
      return false;
    }
    if (destination.write(chunk) != chunk.size()) {
      error = QString("Cannot save the output PDF: %1")
                  .arg(destination.errorString());
      return false;
    }
  }
  if (cancelled) {
    return false;
  }
  if (!destination.commit()) {
    error = QString("Cannot commit the output PDF: %1")
                .arg(destination.errorString());
    return false;
  }
  return true;
}

QImage renderPdfPageOnWhite(QPdfDocument &doc, int pageIndex,
                            const QSize &pixelSize,
                            QImage::Format format) {
  if (pixelSize.width() <= 0 || pixelSize.height() <= 0) {
    return QImage();
  }
  const QImage rendered = doc.render(pageIndex, pixelSize);
  if (rendered.isNull()) {
    return QImage();
  }
  QImage canvas(pixelSize, format);
  canvas.fill(Qt::white);
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.drawImage(0, 0, rendered);
  painter.end();
  return canvas;
}

bool runGhostscriptProcess(const QString &executable, const QStringList &args,
                           const QString &workingDir,
                           const QString &expectedOutputPath,
                           const std::atomic<bool> &cancelled, QString &error,
                           bool failOnWarnings) {
  QProcess proc;
  proc.setWorkingDirectory(workingDir);
  proc.setProcessChannelMode(QProcess::MergedChannels);
  QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
  environment.remove("GS_OPTIONS");
  proc.setProcessEnvironment(environment);
  proc.start(executable, args);
  if (!proc.waitForStarted(5000)) {
    error = QString("Cannot start Ghostscript: %1. Check the bundled runtime "
                    "and its DLLs/resources, or reinstall Ghostscript.")
                .arg(proc.errorString());
    return false;
  }

  QElapsedTimer timer;
  timer.start();
  QByteArray diagnostics;
  while (proc.state() != QProcess::NotRunning) {
    proc.waitForFinished(150);
    diagnostics = (diagnostics + proc.readAll()).right(16384);
    if (cancelled || timer.elapsed() > 15 * 60 * 1000) {
      proc.kill();
      proc.waitForFinished(3000);
      if (!cancelled) {
        error = "Ghostscript exceeded the 15-minute processing limit.";
      }
      return false;
    }
  }
  diagnostics = (diagnostics + proc.readAll()).right(16384);
  if (cancelled) {
    return false;
  }
  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
    error = QString("Ghostscript failed (exit %1): %2")
                .arg(proc.exitCode())
                .arg(QString::fromLocal8Bit(diagnostics).trimmed().right(4096));
    qWarning().noquote() << error;
    return false;
  }
  if (failOnWarnings && !diagnostics.trimmed().isEmpty()) {
    error = QString("Ghostscript reported warnings; the output was not saved "
                    "to avoid accepting missing or substituted content: %1")
                .arg(QString::fromLocal8Bit(diagnostics).trimmed().right(4096));
    qWarning().noquote() << error;
    return false;
  }
  if (QFileInfo(expectedOutputPath).size() <= 0) {
    error = "Ghostscript did not produce a nonempty PDF.";
    return false;
  }
  return true;
}

QString findGhostscriptExecutable() {
  // Single-file packagers can point to their extracted runtime explicitly.
  const QString configured = qEnvironmentVariable("IMAGECONVERTER_GHOSTSCRIPT");
  if (!configured.isEmpty()) {
    const QFileInfo executable(configured);
    return executable.isFile() && executable.isExecutable()
               ? executable.absoluteFilePath()
               : QString();
  }

  const QDir appDir(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
  const QStringList bundledNames = {"gs/bin/gswin64c.exe", "gswin64c.exe"};
  const QStringList systemNames = {"gswin64c", "gswin32c"};
#else
  const QStringList bundledNames = {"gs/bin/gs"};
  const QStringList systemNames = {"gs"};
#endif
  for (const QString &name : bundledNames) {
    const QFileInfo executable(appDir.filePath(name));
    if (executable.isFile() && executable.isExecutable()) {
      return executable.absoluteFilePath();
    }
  }
  for (const QString &name : systemNames) {
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
      return found;
    }
  }
  return QString();
}

bool parsePageRanges(const QString &rangeSpec, int pageCount,
                     QVector<QPair<int, int>> &outRanges,
                     QString &error) {
  outRanges.clear();
  const QString trimmed = rangeSpec.trimmed();
  if (trimmed.isEmpty()) {
    error = "Please enter at least one page or range (e.g. 1-3, 5, 8-12).";
    return false;
  }

  const QStringList tokens = trimmed.split(',', Qt::SkipEmptyParts);
  if (tokens.isEmpty()) {
    error = "Please enter at least one page or range (e.g. 1-3, 5, 8-12).";
    return false;
  }

  for (const QString &rawToken : tokens) {
    const QString token = rawToken.trimmed();
    if (token.isEmpty()) {
      continue;
    }
    if (token.contains('-')) {
      const QStringList parts = token.split('-');
      if (parts.size() != 2) {
        error = QString("Invalid range syntax '%1'.").arg(token);
        return false;
      }
      bool okStart = false;
      bool okEnd = false;
      const int start = parts[0].trimmed().toInt(&okStart);
      const int end = parts[1].trimmed().toInt(&okEnd);
      if (!okStart || !okEnd || start < 1 || end < start || end > pageCount) {
        error = QString("Page range '%1' is invalid (document has %2 page(s)).")
                    .arg(token)
                    .arg(pageCount);
        return false;
      }
      outRanges.append(qMakePair(start, end));
    } else {
      bool okPage = false;
      const int page = token.toInt(&okPage);
      if (!okPage || page < 1 || page > pageCount) {
        error =
            QString("Page number '%1' is invalid (document has %2 page(s)).")
                .arg(token)
                .arg(pageCount);
        return false;
      }
      outRanges.append(qMakePair(page, page));
    }
  }

  if (outRanges.isEmpty()) {
    error = "Please enter at least one valid page number.";
    return false;
  }
  return true;
}

} // namespace pdfutil

