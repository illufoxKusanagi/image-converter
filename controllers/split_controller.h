#ifndef SPLIT_CONTROLLER_H
#define SPLIT_CONTROLLER_H

#include "controllers/pdf_tool_controller.h"
#include <QPdfDocument>
#include <QString>
#include <QStringList>

class SplitController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QString sourcePath;
    QStringList sourcePaths;
    QString outputDir;
    int mode{0}; // 0: Custom ranges, 1: Fixed interval, 2: Extract all
    QString rangeSpec;
    int interval{1};
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    int createdCount{0};
    int failedDocs{0};
    QString outputDir;
    QString error;
    QStringList errors;
    QStringList generatedFiles;
  };

  explicit SplitController(QObject *parent = nullptr);
  ~SplitController() override { cancelAndWait(); }

  void start(const Params &params);

  bool extractPageRangeWithGhostscript(const QString &executable,
                                       const QString &inputPath, int firstPage,
                                       int lastPage,
                                       const QString &tempOutputPath,
                                       QString &error);

  bool extractPageRangeWithQtPdf(QPdfDocument &sourceDoc, int firstPage,
                                 int lastPage, const QString &tempOutputPath,
                                 QString &error);

signals:
  void finished(const SplitController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(SplitController::Result)

#endif // SPLIT_CONTROLLER_H

