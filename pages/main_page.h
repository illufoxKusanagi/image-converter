#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

#include "controllers/pdf/pdf_tool_controller.h"
#include "pages/image/base64_panel.h"
#include "pages/image/convert_image_panel.h"
#include "pages/image/ico_packager_panel.h"
#include "pages/image/svg_options_panel.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/tabs.h"
#include "widgets/drop_file_widget.h"
#include <QLabel>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

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
  void onControllerProgress(int current, int total, const QString &statusText);
  void onControllerFileStatus(const QString &filePath, int status,
                              const QString &detail);
  void applyThemeStyles();

private:
  QVBoxLayout *mainLayout{nullptr};
  ui::Tabs *m_toolTabs{nullptr};
  ImageToolMode m_currentTool{ImageToolMode::ConvertImage};

  QStackedWidget *m_topWorkspaceStack{nullptr};
  DropFileWidget *m_dragWidget{nullptr};
  DropFileWidget *m_svgDragWidget{nullptr};
  Base64WorkspaceCard *m_base64Card{nullptr};

  QStackedWidget *m_optionsStack{nullptr};
  ConvertImagePanel *m_convertImagePanel{nullptr};
  SvgOptionsPanel *m_svgPanel{nullptr};
  IcoPackagerPanel *m_icoPanel{nullptr};
  Base64OptionsPanel *m_base64Panel{nullptr};

  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};

  PdfToolController *m_currentController{nullptr};
  bool m_isCancelled{false};
  bool m_isProcessing{false};

  DropFileWidget *activeDropWidget() const;
  bool isAnyJobRunning() const;
  void initControllers();
  void setupImageLayout();
  void setupToolPanels();
  void updateToolModeUi();
  void setProcessingState(bool isProcessing);
  QString processButtonTextForMode(ImageToolMode mode) const;
  QString cancelButtonTextForMode(ImageToolMode mode) const;
};

#endif // MAIN_PAGE_H
