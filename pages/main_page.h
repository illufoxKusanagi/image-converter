#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

#include "controllers/image/ico_export_controller.h"
#include "controllers/image/svg_convert_controller.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/switch.h"
#include "ui-kit/components/tabs.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QIntValidator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QSet>
#include <QStackedWidget>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <atomic>

class MainPage : public QWidget {
  Q_OBJECT

public:
  enum class ImageToolMode {
    ConvertImage = 0,
    SvgToRaster,
    IcoPackager,
    Base64DataUri
  };

  explicit MainPage(QWidget *parent = nullptr);
  ~MainPage() override;

private slots:
  void onToolTabChanged(int index);
  void onProcessButtonClicked();
  void onCancelButtonClicked();
  void onImageTargetExtensionChanged();
  void onSvgFormatChanged(int index);
  void onSvgScaleModeChanged(int index);
  void onBase64LoadFileClicked();
  void onBase64CopyClicked();
  void onBase64DecodeSaveClicked();
  void onBase64OptionChanged();
  void onControllerProgress(int current, int total, const QString &statusText);
  void onControllerFileStatus(const QString &filePath, int status,
                              const QString &detail);
  void onSvgConvertFinished(const SvgConvertController::Result &result);
  void onIcoExportFinished(const IcoExportController::Result &result);
  void applyThemeStyles();

private:
  QVBoxLayout *mainLayout{nullptr};
  ui::Tabs *m_toolTabs{nullptr};
  ImageToolMode m_currentTool{ImageToolMode::ConvertImage};

  QStackedWidget *m_topWorkspaceStack{nullptr};
  DropFileWidget *m_dragWidget{nullptr};
  DropFileWidget *m_svgDragWidget{nullptr};
  ui::Card *m_base64Card{nullptr};

  QStackedWidget *m_optionsStack{nullptr};

  // Mode 0: Convert Image controls
  QWidget *m_convertImagePanel{nullptr};
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_targetExtensionSelect{nullptr};
  ui::FormField *m_targetExtensionField{nullptr};

  // Mode 1: SVG Vector controls
  QWidget *m_svgPanel{nullptr};
  ui::Select *m_svgFormatSelect{nullptr};
  ui::FormField *m_svgFormatField{nullptr};
  ui::Select *m_svgScaleSelect{nullptr};
  ui::FormField *m_svgScaleField{nullptr};
  ui::Input *m_svgCustomWidthInput{nullptr};
  ui::FormField *m_svgCustomWidthField{nullptr};
  ui::Input *m_svgCustomHeightInput{nullptr};
  ui::FormField *m_svgCustomHeightField{nullptr};
  SliderWidget *m_svgQualitySlider{nullptr};
  ui::Switch *m_svgKeepAspectSwitch{nullptr};
  QLabel *m_svgKeepAspectLabel{nullptr};
  ui::Switch *m_svgWhiteBgSwitch{nullptr};
  QLabel *m_svgWhiteBgLabel{nullptr};

  // Mode 2: App Icon (.ico) controls
  QWidget *m_icoPanel{nullptr};
  QLabel *m_icoHintLabel{nullptr};
  struct IcoSizeToggle {
    int size{0};
    ui::Switch *toggle{nullptr};
    QLabel *label{nullptr};
  };
  QVector<IcoSizeToggle> m_icoSizeToggles;

  // Mode 3: Base64 Data URI controls
  QWidget *m_base64Panel{nullptr};
  ui::Select *m_base64SnippetSelect{nullptr};
  ui::FormField *m_base64SnippetField{nullptr};
  ui::Select *m_base64EncodeSelect{nullptr};
  ui::FormField *m_base64EncodeField{nullptr};
  ui::Button *m_base64LoadBtn{nullptr};
  ui::Button *m_base64CopyBtn{nullptr};
  ui::Button *m_base64DecodeSaveBtn{nullptr};
  QLabel *m_base64StatsLabel{nullptr};
  QPlainTextEdit *m_base64TextEdit{nullptr};
  QString m_base64LoadedFilePath;

  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};

  QFutureWatcher<void> *m_futureWatcher{nullptr};
  SvgConvertController *m_svgController{nullptr};
  IcoExportController *m_icoController{nullptr};
  PdfToolController *m_currentController{nullptr};

  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};
  DropFileWidget::ImageExtension m_targetImageExtension;

  DropFileWidget *activeDropWidget() const;
  bool isAnyJobRunning() const;
  void initControllers();
  void setupImageLayout();
  void setupExtensionButton();
  void setupToolPanels();
  void updateToolModeUi();
  void setProcessingState(bool isProcessing);
  QString processButtonTextForMode(ImageToolMode mode) const;
  QString cancelButtonTextForMode(ImageToolMode mode) const;

  void executeConvertImage();
  void executeSvgConvert();
  void executeIcoExport();
  void loadFileIntoBase64(const QString &filePath);
};

#endif // MAIN_PAGE_H
