#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/select.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QLabel>
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
  DropFileWidget *m_dragWidget;
  SliderWidget *m_qualitySlider;
  ui::Select *m_targetExtensionSelect{nullptr};
  ui::FormField *m_targetExtensionField{nullptr};
  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton;
  ui::Button *m_cancelButton;
  QFutureWatcher<void> *m_futureWatcher;
  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};
  DropFileWidget::ImageExtension m_targetImageExtension;

  void setupImageLayout();
  void setupExtensionButton();
  void setupImageAttribute();
  void setProcessingState(bool isProcessing);
};

#endif // MAIN_PAGE_H
