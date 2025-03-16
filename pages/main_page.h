#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H
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
  void setupImageInput();
};

#endif // MAIN_PAGE_H
