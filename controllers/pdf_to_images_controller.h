#ifndef PDF_TO_IMAGES_CONTROLLER_H
#define PDF_TO_IMAGES_CONTROLLER_H

#include "controllers/pdf_tool_controller.h"
#include <QString>
#include <QStringList>

class PdfToImagesController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QStringList sourcePaths;
    QString outputDir;
    QString format{"png"};
    int dpi{300};
    QString rangeFilter;
    int quality{50};
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    int imagesExported{0};
    int failedDocs{0};
    QString outputDir;
    QString format;
    int dpi{300};
    QString error;
    QStringList errors;
  };

  explicit PdfToImagesController(QObject *parent = nullptr);
  ~PdfToImagesController() override { cancelAndWait(); }

  void start(const Params &params);

signals:
  void finished(const PdfToImagesController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(PdfToImagesController::Result)

#endif // PDF_TO_IMAGES_CONTROLLER_H

