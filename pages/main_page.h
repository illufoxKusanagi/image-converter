#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>

class MainPage : public QWidget {
  Q_OBJECT

public:
  explicit MainPage(QWidget *parent = nullptr);
  ~MainPage() override;

private slots:
  void onProcessButtonClicked();
  void onCancelButtonClicked();
  void onImageTargetExtensionChanged();
  void applyThemeStyles();

private:
  QVBoxLayout *mainLayout;
  SliderWidget *m_qualitySlider;
  DropFileWidget *m_dragWidget;
  ui::FormField *m_targetExtensionField;
  ui::Select *m_targetExtensionSelect;
  QProgressBar *m_progressBar;
  ui::Button *m_processButton;
  ui::Button *m_cancelButton;
  DropFileWidget::ImageExtension m_targetImageExtension;

  QFutureWatcher<void> *m_futureWatcher;
  std::atomic<bool> m_isProcessing{false};
  std::atomic<bool> m_isCancelled{false};

  void setupExtensionButton();
  void setupImageLayout();
  void setupImageAttribute();
  void setProcessingState(bool isProcessing);
};

#endif // MAIN_PAGE_H
