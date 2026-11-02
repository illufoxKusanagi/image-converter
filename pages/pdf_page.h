#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/number_input.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/tabs.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QLabel>
#include <QListWidget>
#include <QPair>
#include <QPdfDocument>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <atomic>

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
    Grayscale
  };

  explicit PdfPage(QWidget *parent = nullptr);
  ~PdfPage() override;

private slots:
  void onToolTabChanged(int index);
  void onProcessButtonClicked();
  void onCancelButtonClicked();
  void onPresetChanged(int index);
  void onQualitySliderChanged();
  void onSplitModeChanged(int index);
  void onExportFormatChanged(int index);
  void applyThemeStyles();

  // Merge tool slots
  void syncMergeListFromDropWidget();
  void moveMergeItemUp();
  void moveMergeItemDown();

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

private:
  QVBoxLayout *mainLayout{nullptr};
  ui::Tabs *m_toolTabs{nullptr};
  PdfToolMode m_currentTool{PdfToolMode::Compress};

  DropFileWidget *m_dragWidget{nullptr};
  DropFileWidget *m_imageDragWidget{nullptr};

  QWidget *m_qualityRowWidget{nullptr};
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_presetSelect{nullptr};
  ui::FormField *m_presetField{nullptr};

  QStackedWidget *m_optionsStack{nullptr};

  // Merge PDF controls
  QWidget *m_mergePanel{nullptr};
  QLabel *m_mergeHintLabel{nullptr};
  QListWidget *m_mergeOrderList{nullptr};
  ui::Button *m_mergeSyncBtn{nullptr};
  ui::Button *m_mergeUpBtn{nullptr};
  ui::Button *m_mergeDownBtn{nullptr};

  // Split PDF controls
  QWidget *m_splitPanel{nullptr};
  ui::Select *m_splitModeSelect{nullptr};
  ui::FormField *m_splitModeField{nullptr};
  ui::Input *m_splitRangesInput{nullptr};
  ui::FormField *m_splitRangesField{nullptr};
  ui::NumberInput *m_splitIntervalInput{nullptr};
  ui::FormField *m_splitIntervalField{nullptr};

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

  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};
  QFutureWatcher<void> *m_futureWatcher{nullptr};
  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};

  DropFileWidget *activeDropWidget() const;
  void setupToolPanels();
  void updateToolModeUi();
  void setProcessingState(bool isProcessing);
  QString processButtonTextForMode(PdfToolMode mode) const;
  QString cancelButtonTextForMode(PdfToolMode mode) const;
  void updateOrganizeItemDisplay(QListWidgetItem *item) const;

  enum class CompressionStatus { Compressed, Unchanged, Failed, Cancelled };
  struct CompressionResult {
    CompressionStatus status{CompressionStatus::Failed};
    qint64 originalBytes{0};
    qint64 outputBytes{0};
    QString error;
  };

  struct PageOp {
    int originalPageIndex{0};
    int rotationDegrees{0};
  };

  // Compress PDF (Tool 0)
  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  CompressionResult compressPdf(const QString &inputPath,
                                const QString &outputPath, int quality);
  bool compressPdfWithGhostscript(const QString &executable,
                                  const QString &inputPath,
                                  const QString &tempOutputPath, int quality,
                                  QString &error, bool grayscale = false);
  static QString findGhostscriptExecutable();

  // Merge PDF (Tool 1)
  void mergePdfs();
  bool mergePdfsWithGhostscript(const QString &executable,
                                const QStringList &inputPaths,
                                const QString &tempOutputPath, QString &error);
  bool mergePdfsWithQtPdf(const QStringList &inputPaths,
                          const QString &tempOutputPath, QString &error);

  // Split PDF (Tool 2)
  void splitPdfs();
  static bool parsePageRanges(const QString &rangeSpec, int pageCount,
                              QVector<QPair<int, int>> &outRanges,
                              QString &error);
  bool extractPageRangeWithGhostscript(const QString &executable,
                                       const QString &inputPath, int firstPage,
                                       int lastPage,
                                       const QString &tempOutputPath,
                                       QString &error);
  bool extractPageRangeWithQtPdf(QPdfDocument &sourceDoc, int firstPage,
                                 int lastPage, const QString &tempOutputPath,
                                 QString &error);

  // Organize / Rotate / Delete PDF Pages (Tool 3)
  void organizePdf();

  // PDF to Images (Tool 4)
  void exportPdfToImages();

  // Images to Merged PDF (Tool 5)
  void imagesToPdf();

  // PDF to Grayscale (Tool 6)
  void convertSinglePdfToGrayscale(const QString &sourcePath, int quality);
  void convertBatchPdfToGrayscale(const QStringList &sourcePaths, int quality);
  CompressionResult convertPdfToGrayscale(const QString &inputPath,
                                          const QString &outputPath,
                                          int quality);
  bool convertPdfToGrayscaleWithQtPdf(QPdfDocument &sourceDoc,
                                      const QString &tempOutputPath,
                                      int quality, QString &error);
};

#endif // PDF_PAGE_H
