#include "main_page.h"

MainPage::MainPage(QWidget *parent) : QWidget(parent) {
  QGridLayout *layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  QGroupBox *groupBox = new QGroupBox("Main Page", this);
  groupBox->setStyleSheet("QGroupBox {"
                          "    border: 0 solid;"
                          "}"
                          "QGroupBox::title {"
                          "    subcontrol-origin: margin;"
                          "    subcontrol-position: top center;"
                          "    padding: 0 4px;"
                          "    background-color: " +
                          Colors::Grey200.name() +
                          ";"
                          "    color: " +
                          Colors::Grey800.name() +
                          ";"
                          "    " +
                          TextStyle::BodyMediumBold() + "}");
  layout->addWidget(groupBox, 0, 0, 1, 1);
  InputType inputType = InputType("field", "contoh");

  InputWidget *inputWidget = new InputWidget(inputType, this);
  layout->addWidget(inputWidget, 1, 0, 1, 1);
}