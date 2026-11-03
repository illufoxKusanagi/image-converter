#ifndef PDF_UTILS_H
#define PDF_UTILS_H

#include <QFileInfo>
#include <QImage>
#include <QPair>
#include <QPdfDocument>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>

namespace pdfutil {

bool isSamePdf(const QFileInfo &source, const QFileInfo &destination);

bool savePdfAtomically(const QString &candidatePath, const QString &outputPath,
                       const std::atomic<bool> &cancelled, QString &error);

QImage renderPdfPageOnWhite(QPdfDocument &doc, int pageIndex,
                            const QSize &pixelSize,
                            QImage::Format format = QImage::Format_RGB32);

bool runGhostscriptProcess(const QString &executable, const QStringList &args,
                           const QString &workingDir,
                           const QString &expectedOutputPath,
                           const std::atomic<bool> &cancelled, QString &error,
                           bool failOnWarnings = true);

QString findGhostscriptExecutable();

bool parsePageRanges(const QString &rangeSpec, int pageCount,
                     QVector<QPair<int, int>> &outRanges, QString &error);

} // namespace pdfutil

#endif // PDF_UTILS_H

