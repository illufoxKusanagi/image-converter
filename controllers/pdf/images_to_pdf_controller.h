#ifndef IMAGES_TO_PDF_CONTROLLER_H
#define IMAGES_TO_PDF_CONTROLLER_H

#include "controllers/pdf/pdf_tool_controller.h"
#include <QString>
#include <QStringList>

class ImagesToPdfController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QStringList imagePaths;
    QString outputPath;
    int pageSizeMode{0};     // 0: Fit to Image, 1: A4, 2: Letter
    int orientationMode{0};  // 0: Auto, 1: Portrait, 2: Landscape
    int marginMode{0};       // 0: 0pt, 1: 18pt, 2: 36pt, 3: 54pt
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    QString outputPath;
    int pageCount{0};
    qint64 outputBytes{0};
    QString error;
  };

  explicit ImagesToPdfController(QObject *parent = nullptr);
  ~ImagesToPdfController() override { cancelAndWait(); }

  void start(const Params &params);

signals:
  void finished(const ImagesToPdfController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(ImagesToPdfController::Result)

#endif // IMAGES_TO_PDF_CONTROLLER_H
