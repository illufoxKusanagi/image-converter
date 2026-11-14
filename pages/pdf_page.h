#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "controllers/pdf/pdf_tool_controller.h"
#include "pages/pdf/images_to_pdf_panel.h"
#include "pages/pdf/markdown_pdf_panel.h"
#include "pages/pdf/merge_pdf_panel.h"
#include "pages/pdf/organize_pdf_panel.h"
#include "pages/pdf/pdf_quality_panel.h"
#include "pages/pdf/pdf_to_images_panel.h"
#include "pages/pdf/split_pdf_panel.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/tabs.h"
#include "widgets/drop_file_widget.h"
#include <QLabel>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class PdfPage : public QWidget {
  Q_OBJECT

public:
  enum class PdfToolMode {
    Compress = 0,
    Merge,
    Split,
    Organize,
    PdfToImages,
    ImagesToPdf,
    Grayscale,
    MarkdownToPdf
  };

  explicit PdfPage(QWidget *parent = nullptr);
  ~PdfPage() override;

private slots:
  void onToolTabChanged(int index);
  void onProcessButtonClicked();
  void onCancelButtonClicked();
  void applyThemeStyles();

  // Controller signal handlers
  void onControllerProgress(int current, int total, const QString &statusText);
  void onControllerFileStatus(const QString &filePath, int status, const QString &detail);

private:
  QVBoxLayout *mainLayout{nullptr};
  ui::Tabs *m_toolTabs{nullptr};
  PdfToolMode m_currentTool{PdfToolMode::Compress};

  DropFileWidget *m_dragWidget{nullptr};
  DropFileWidget *m_imageDragWidget{nullptr};
  DropFileWidget *m_docDragWidget{nullptr};

  PdfQualityPanel *m_qualityPanel{nullptr};

  QStackedWidget *m_optionsStack{nullptr};

  // Merge PDF controls
  MergePdfPanel *m_mergePanel{nullptr};

  // Split PDF controls
  SplitPdfPanel *m_splitPanel{nullptr};

  // Organize / Rotate / Delete PDF controls
  OrganizePdfPanel *m_organizePanel{nullptr};

  // PDF to Images controls
  PdfToImagesPanel *m_pdfToImagesPanel{nullptr};

  // Images to PDF controls
  ImagesToPdfPanel *m_imagesToPdfPanel{nullptr};

  // Markdown / HTML to PDF controls
  MarkdownPdfPanel *m_markdownPanel{nullptr};

  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};

  bool m_isProcessing{false};

  // Controllers
  PdfToolController *m_currentController{nullptr};

  DropFileWidget *activeDropWidget() const;
  void setupToolPanels();
  void updateToolModeUi();
  void setProcessingState(bool isProcessing);
  QString processButtonTextForMode(PdfToolMode mode) const;
  QString cancelButtonTextForMode(PdfToolMode mode) const;

  void initControllers();
};

#endif // PDF_PAGE_H
