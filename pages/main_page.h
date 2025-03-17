#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
#include "widgets/button_action.h"
#include "widgets/input_widget.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QStackedWidget>

class MainPage : public QWidget {
  Q_OBJECT
public:
  explicit MainPage(QWidget *parent = nullptr);

private:
  QVBoxLayout *mainLayout;
  QHBoxLayout *m_imageLayout;
  void setupImageInput();
  void setupExtensionButton();
  void setupImageLayout();
};

#endif // MAIN_PAGE_H
