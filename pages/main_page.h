#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include "widgets/button_action.h"
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
#include "widgets/slider_widget.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QStackedWidget>

/**
 * @brief Handles the event when the process button is clicked.
 *
 * This slot is triggered in response to the user clicking the process button, initiating the main processing action for the page.
 */
class MainPage : public QWidget {
  Q_OBJECT

private slots:
  void onProcessButtonClicked();

public:
  explicit MainPage(QWidget *parent = nullptr);

private:
  QVBoxLayout *mainLayout;
  QVBoxLayout *m_imageLayout;
  SliderWidget *m_qualitySlider;
  DropFileWidget *m_dropFileWidget;
  QWidget *m_buttonWidget;
  QVBoxLayout *m_buttonLayout;
  DropFileWidget *m_dragWidget;
  void setupImageInput();
  void setupExtensionButton();
  void setupImageLayout();
  void setupQualitySlider();
  void setupImageAttribute();
};

#endif // MAIN_PAGE_H
