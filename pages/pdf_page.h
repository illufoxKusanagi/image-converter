#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "controllers/pdf/compress_controller.h"
#include "controllers/pdf/grayscale_controller.h"
#include "controllers/pdf/images_to_pdf_controller.h"
#include "controllers/pdf/markdown_pdf_controller.h"
#include "controllers/pdf/merge_controller.h"
#include "controllers/pdf/organize_controller.h"
#include "controllers/pdf/pdf_to_images_controller.h"
#include "controllers/pdf/split_controller.h"
#include "pages/pdf/merge_pdf_panel.h"
#include "pages/pdf/split_pdf_panel.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/tabs.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
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
  void onPresetChanged(int index);
  void onQualitySliderChanged();
  void onExportFormatChanged(int index);
  void applyThemeStyles();

  // Merge tool slots
  void syncMergeListFromDropWidget();

  // Organize tool slots
  void loadOrganizePagesFromSelectedPdf(bool silentIfEmpty = false);
  void rotateSelectedOrganizePages(int deltaDegrees);
  void moveSelectedOrganizePage(int direction);
  void deleteSelectedOrganizePages();
  void resetOrganizePages();

  // Images to PDF tool slots
  void syncImageListFromDropWidget();
  void moveImageItemUp();
  void moveImageItemDown();

  // Controller signal handlers
  void onControllerProgress(int current, int total, const QString &statusText);
  void onControllerFileStatus(const QString &filePath, int status, const QString &detail);
  void onCompressFinished(const CompressController::Result &result);
  void onMergeFinished(const MergeController::Result &result);
  void onSplitFinished(const SplitController::Result &result);
  void onOrganizeFinished(const OrganizeController::Result &result);
  void onPdfToImagesFinished(const PdfToImagesController::Result &result);
  void onImagesToPdfFinished(const ImagesToPdfController::Result &result);
  void onGrayscaleFinished(const GrayscaleController::Result &result);
  void onMarkdownPdfFinished(const MarkdownPdfController::Result &result);

private:
  QVBoxLayout *mainLayout{nullptr};
  ui::Tabs *m_toolTabs{nullptr};
  PdfToolMode m_currentTool{PdfToolMode::Compress};

  DropFileWidget *m_dragWidget{nullptr};
  DropFileWidget *m_imageDragWidget{nullptr};
  DropFileWidget *m_docDragWidget{nullptr};

  QWidget *m_qualityRowWidget{nullptr};
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_presetSelect{nullptr};
  ui::FormField *m_presetField{nullptr};

  QStackedWidget *m_optionsStack{nullptr};

  // Merge PDF controls
  MergePdfPanel *m_mergePanel{nullptr};

  // Split PDF controls
  SplitPdfPanel *m_splitPanel{nullptr};

  // Organize / Rotate / Delete PDF controls
  QWidget *m_organizePanel{nullptr};
  QLabel *m_organizeHintLabel{nullptr};
  QListWidget *m_organizePageList{nullptr};
  ui::Button *m_orgLoadBtn{nullptr};
  ui::Button *m_orgRotLeftBtn{nullptr};
  ui::Button *m_orgRotRightBtn{nullptr};
  ui::Button *m_orgRot180Btn{nullptr};
  ui::Button *m_orgMoveUpBtn{nullptr};
  ui::Button *m_orgMoveDownBtn{nullptr};
  ui::Button *m_orgDeleteBtn{nullptr};
  ui::Button *m_orgResetBtn{nullptr};
  QString m_organizedSourcePath;

  // PDF to Images controls
  QWidget *m_pdfToImagesPanel{nullptr};
  ui::Select *m_exportImageFormatSelect{nullptr};
  ui::FormField *m_exportImageFormatField{nullptr};
  ui::Select *m_exportDpiSelect{nullptr};
  ui::FormField *m_exportDpiField{nullptr};
  ui::Input *m_exportPageRangeInput{nullptr};
  ui::FormField *m_exportPageRangeField{nullptr};

  // Images to PDF controls
  QWidget *m_imagesToPdfPanel{nullptr};
  QLabel *m_imageOrderHintLabel{nullptr};
  QListWidget *m_imageOrderList{nullptr};
  ui::Button *m_imgSyncBtn{nullptr};
  ui::Button *m_imgUpBtn{nullptr};
  ui::Button *m_imgDownBtn{nullptr};
  ui::Select *m_imgPdfPageSizeSelect{nullptr};
  ui::FormField *m_imgPdfPageSizeField{nullptr};
  ui::Select *m_imgPdfOrientationSelect{nullptr};
  ui::FormField *m_imgPdfOrientationField{nullptr};
  ui::Select *m_imgPdfMarginSelect{nullptr};
  ui::FormField *m_imgPdfMarginField{nullptr};

  // Markdown / HTML to PDF controls
  QWidget *m_markdownPanel{nullptr};
  ui::Select *m_mdPageSizeSelect{nullptr};
  ui::FormField *m_mdPageSizeField{nullptr};
  ui::Select *m_mdOrientationSelect{nullptr};
  ui::FormField *m_mdOrientationField{nullptr};
  ui::Select *m_mdMarginSelect{nullptr};
  ui::FormField *m_mdMarginField{nullptr};
  ui::Select *m_mdFontSizeSelect{nullptr};
  ui::FormField *m_mdFontSizeField{nullptr};

  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};

  bool m_isProcessing{false};

  // Controllers
  CompressController *m_compressController{nullptr};
  MergeController *m_mergeController{nullptr};
  SplitController *m_splitController{nullptr};
  OrganizeController *m_organizeController{nullptr};
  PdfToImagesController *m_pdfToImagesController{nullptr};
  ImagesToPdfController *m_imagesToPdfController{nullptr};
  GrayscaleController *m_grayscaleController{nullptr};
  MarkdownPdfController *m_markdownController{nullptr};
  PdfToolController *m_currentController{nullptr};

  DropFileWidget *activeDropWidget() const;
  void setupToolPanels();
  void updateToolModeUi();
  void setProcessingState(bool isProcessing);
  QString processButtonTextForMode(PdfToolMode mode) const;
  QString cancelButtonTextForMode(PdfToolMode mode) const;
  void updateOrganizeItemDisplay(QListWidgetItem *item) const;

  void initControllers();
  void executeCompress();
  void executeMerge();
  void executeSplit();
  void executeOrganize();
  void executePdfToImages();
  void executeImagesToPdf();
  void executeGrayscale();
  void executeMarkdownPdf();
};

#endif // PDF_PAGE_H
