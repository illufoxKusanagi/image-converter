#ifndef MERGE_CONTROLLER_H
#define MERGE_CONTROLLER_H

#include "controllers/pdf/pdf_tool_controller.h"
#include <QString>
#include <QStringList>

class MergeController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QStringList inputPaths;
    QString outputPath;
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    QString outputPath;
    int totalPages{0};
    qint64 outputBytes{0};
    QString error;
    QString warningDetail;
  };

  explicit MergeController(QObject *parent = nullptr);
  ~MergeController() override { cancelAndWait(); }

  void start(const Params &params);

  bool mergePdfsWithGhostscript(const QString &executable,
                                const QStringList &inputPaths,
                                const QString &tempOutputPath,
                                QString &error);

  bool mergePdfsWithQtPdf(const QStringList &inputPaths,
                          const QString &tempOutputPath,
                          QString &error);

signals:
  void finished(const MergeController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(MergeController::Result)

#endif // MERGE_CONTROLLER_H
