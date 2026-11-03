#ifndef ORGANIZE_CONTROLLER_H
#define ORGANIZE_CONTROLLER_H

#include "controllers/pdf_tool_controller.h"
#include <QString>
#include <QVector>

class OrganizeController : public PdfToolController {
  Q_OBJECT

public:
  struct PageOp {
    int originalPageIndex{0};
    int rotationDegrees{0};
  };

  struct Params {
    QString sourcePath;
    QString outputPath;
    QVector<PageOp> pageOps;
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    QString outputPath;
    int pageCount{0};
    qint64 outputBytes{0};
    QString error;
  };

  explicit OrganizeController(QObject *parent = nullptr);
  ~OrganizeController() override { cancelAndWait(); }

  void start(const Params &params);

signals:
  void finished(const OrganizeController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(OrganizeController::PageOp)
Q_DECLARE_METATYPE(OrganizeController::Result)

#endif // ORGANIZE_CONTROLLER_H

