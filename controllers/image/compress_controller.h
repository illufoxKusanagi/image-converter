#ifndef COMPRESS_CONTROLLER_H
#define COMPRESS_CONTROLLER_H

#include "controllers/pdf_tool_controller.h"
#include <QFileInfo>
#include <QString>
#include <QStringList>

class CompressController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    bool isBatch{false};
    QString sourcePath;
    QString outputPath;
    QStringList sourcePaths;
    QString outputDir;
    int quality{50};
  };

  struct Result {
    enum class Status {
      Compressed,
      Unchanged,
      Failed,
      Cancelled
    };
    bool isBatch{false};
    Status status{Status::Failed};
    qint64 origBytes{0};
    qint64 outBytes{0};
    int successCount{0};
    int unchangedCount{0};
    int failCount{0};
    QStringList errors;
    QString singleOutputPath;
  };

  struct SingleResult {
    Result::Status status{Result::Status::Failed};
    qint64 originalBytes{0};
    qint64 outputBytes{0};
    QString error;
  };

  explicit CompressController(QObject *parent = nullptr);
  ~CompressController() override { cancelAndWait(); }

  void start(const Params &params);

  static bool compressPdfWithGhostscript(const QString &executable,
                                         const QString &inputPath,
                                         const QString &tempOutputPath,
                                         int quality, QString &error,
                                         bool grayscale = false,
                                         const std::atomic<bool> *cancelled = nullptr);

  SingleResult compressPdf(const QString &inputPath, const QString &outputPath,
                           int quality);

signals:
  void finished(const CompressController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(CompressController::Result)

#endif // COMPRESS_CONTROLLER_H

