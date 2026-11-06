#ifndef MARKDOWN_PDF_CONTROLLER_H
#define MARKDOWN_PDF_CONTROLLER_H

#include "controllers/pdf/pdf_tool_controller.h"
#include <QString>
#include <QStringList>

class MarkdownPdfController : public PdfToolController {
  Q_OBJECT

public:
  struct Params {
    QStringList sourcePaths;
    QString outputDir;
    bool useLetterPageSize{false}; // false = A4, true = Letter
    bool landscape{false};
    int marginPt{36};    // 18, 36, 54, 72 pt
    int baseFontPt{10};  // 9..16 pt
  };

  struct Result {
    bool success{false};
    bool cancelled{false};
    int pdfsCreated{0};
    int failedFiles{0};
    QString outputDir;
    QString error;
    QStringList errors;
  };

  explicit MarkdownPdfController(QObject *parent = nullptr);
  ~MarkdownPdfController() override { cancelAndWait(); }

  void start(const Params &params);

signals:
  void finished(const MarkdownPdfController::Result &result);

private:
  void runJob(const Params &params);
};

Q_DECLARE_METATYPE(MarkdownPdfController::Result)

#endif // MARKDOWN_PDF_CONTROLLER_H
