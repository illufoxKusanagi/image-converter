#ifndef UI_DEMO_PAGE_H
#define UI_DEMO_PAGE_H

#include <QWidget>

class QScrollArea;
class QVBoxLayout;
class QLabel;

namespace ui {

class Button;
class Spinner;
class Progress;
class Switch;
class Slider;
class Input;

/**
 * @brief Self-contained interactive showcase gallery of all UI-Kit components
 * with dynamic theme reactivity.
 */
class DemoPage : public QWidget {
  Q_OBJECT

public:
  explicit DemoPage(QWidget *parent = nullptr);
  ~DemoPage() override = default;

private slots:
  void applyThemeStyles();
  void onToggleLoadingClicked();
  void onShowToastSuccessClicked();
  void onShowToastDestructiveClicked();
  void onShowDialogInfoClicked();

private:
  void setupUi();
  QWidget *createSectionHeader(const QString &title,
                               const QString &description = "");
  QWidget *buildButtonSection();
  QWidget *buildBadgeSection();
  QWidget *buildFormSection();
  QWidget *buildFeedbackSection();
  QWidget *buildContentSection();
  QWidget *buildSelectionSection();
  QWidget *buildTextNumberSection();
  QWidget *buildActionsSection();
  QWidget *buildStatusSection();
  QWidget *buildLayoutSection();

  QScrollArea *m_scrollArea{nullptr};
  QWidget *m_containerWidget{nullptr};
  QVBoxLayout *m_contentLayout{nullptr};

  Button *m_loadingTestBtn{nullptr};
  Spinner *m_spinner{nullptr};
  Progress *m_progress{nullptr};
  Slider *m_slider{nullptr};
  Switch *m_switch{nullptr};
};

} // namespace ui

#endif // UI_DEMO_PAGE_H
