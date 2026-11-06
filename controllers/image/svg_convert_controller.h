#ifndef SVG_CONVERT_CONTROLLER_H
#define SVG_CONVERT_CONTROLLER_H

#include "controllers/pdf/pdf_tool_controller.h"
#include <QString>
#include <QStringList>

class SvgConvertController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QStringList sourcePaths;
    QString outputDir;
    QString format{"png"}; // "png", "webp", "jpg", "pdf"
    int scaleMode{0};      // 0 = 1x, 1 = 2x, 2 = 4x, 3 = Custom W x H px
    int customWidth{512};
    int customHeight{512};
    bool preserveAspectRatio{true};
    bool whiteBackground{false}; // always true if format == "jpg"
    int quality{90};             // used for webp/jpg; -1 for png
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    int filesConverted{0};
    int failedFiles{0};
    QString outputDir;
    QString format;
    QString error;
    QStringList errors;
  };

  explicit SvgConvertController(QObject *parent = nullptr);
  ~SvgConvertController() override { cancelAndWait(); }

  void start(const Params &params);

signals:
  void finished(const SvgConvertController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(SvgConvertController::Result)

#endif // SVG_CONVERT_CONTROLLER_H
