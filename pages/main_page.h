#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include "widgets/button_action.h"
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
#include "widgets/slider_widget.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QStackedWidget>

class MainPage : public QWidget {
  Q_OBJECT
public:
  explicit MainPage(QWidget *parent = nullptr);

private:
  QVBoxLayout *mainLayout;
  QVBoxLayout *m_imageLayout;
  SliderWidget *m_qualitySlider;
  DropFileWidget *m_dropFileWidget;
  void setupImageInput();
  void setupExtensionButton();
  void setupImageLayout();
  void setupQualitySlider();
};

#endif // MAIN_PAGE_H
