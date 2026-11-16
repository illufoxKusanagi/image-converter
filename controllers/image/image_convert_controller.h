#ifndef IMAGE_CONVERT_CONTROLLER_H
#define IMAGE_CONVERT_CONTROLLER_H

#include "controllers/pdf/pdf_tool_controller.h"
#include "widgets/drop_file_widget.h"
#include <QImage>
#include <QString>
#include <QStringList>

class ImageConvertController : public PdfToolController {
  Q_OBJECT

public:
  enum class Mode {
    ImagesToPdf = 0,
    PdfToImages,
    SingleImage,
    BatchImage
  };

  struct Params {
    Mode mode{Mode::BatchImage};
    QStringList sourcePaths;
    QString outputPath;
    QString outputDir;
    int quality{100};
    DropFileWidget::ImageExtension targetExtension{
        DropFileWidget::ImageExtension::JPG};
    QString targetFormat;
    bool forceOpaque{false};
  };

  struct Result {
    Mode mode{Mode::BatchImage};
    bool success{false};
    bool cancelled{false};
    int successCount{0};
    int failedCount{0};
    int skippedCount{0};
    qint64 totalOrigBytes{0};
    qint64 totalOutputBytes{0};
    QString sourcePath;
    QString outputPath;
    QString outputDir;
    QString error;
  };

  explicit ImageConvertController(QObject *parent = nullptr);
  ~ImageConvertController() override { cancelAndWait(); }

  static QImage loadRasterOrSvgImage(const QString &path,
                                     bool forceOpaqueWhite = false);
  static QString extensionToString(DropFileWidget::ImageExtension ext);
  static bool saveImageToFormat(
      const QImage &image, const QString &outputPath, int quality,
      DropFileWidget::ImageExtension targetExtension);

  void start(const Params &params);

signals:
  void finished(const ImageConvertController::Result &result);

private:
  void runJob(const Params &params);
  Result runImagesToPdf(const Params &params);
  Result runPdfToImages(const Params &params);
  Result runSingleImage(const Params &params);
  Result runBatchImage(const Params &params);
};

Q_DECLARE_METATYPE(ImageConvertController::Result)

#endif // IMAGE_CONVERT_CONTROLLER_H
