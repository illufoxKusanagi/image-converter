#ifndef GRAYSCALE_CONTROLLER_H
#define GRAYSCALE_CONTROLLER_H

#include "controllers/pdf/compress_controller.h"
#include "controllers/pdf/pdf_tool_controller.h"
#include <QPdfDocument>
#include <QString>
#include <QStringList>

class GrayscaleController : public PdfToolController {
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
    bool isBatch{false};
    bool success{false};
    bool cancelled{false};
    int successCount{0};
    int failCount{0};
    qint64 origBytes{0};
    qint64 outBytes{0};
    QString singleOutputPath;
    QString error;
    QStringList errors;
  };

  explicit GrayscaleController(QObject *parent = nullptr);
  ~GrayscaleController() override { cancelAndWait(); }

  void start(const Params &params);

  bool convertPdfToGrayscaleWithQtPdf(QPdfDocument &sourceDoc,
                                      const QString &tempOutputPath,
                                      int quality, QString &error);

  CompressController::SingleResult convertPdfToGrayscale(
      const QString &inputPath, const QString &outputPath, int quality);

signals:
  void finished(const GrayscaleController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(GrayscaleController::Result)

#endif // GRAYSCALE_CONTROLLER_H

