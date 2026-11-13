#ifndef ICO_EXPORT_CONTROLLER_H
#define ICO_EXPORT_CONTROLLER_H

#include "controllers/pdf/pdf_tool_controller.h"
#include <QString>
#include <QStringList>
#include <QVector>

class IcoExportController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QStringList sourcePaths;
    QString outputDir;
    QVector<int> sizes{16, 32, 48, 64, 128, 256};
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    int iconsCreated{0};
    int failedFiles{0};
    QString outputDir;
    QVector<int> sizes;
    QString error;
    QStringList errors;
  };

  explicit IcoExportController(QObject *parent = nullptr);
  ~IcoExportController() override { cancelAndWait(); }

  void start(const Params &params);

signals:
  void finished(const IcoExportController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(IcoExportController::Result)

#endif // ICO_EXPORT_CONTROLLER_H
