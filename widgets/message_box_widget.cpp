#include "message_box_widget.h"

MessageBoxWidget::MessageBoxWidget(QString windowTitle, QString text,
                                   IconType iconType) {
  setWindowTitle(windowTitle);
  setText(text);
  setIcon(switchIcon(iconType));
  setStandardButtons(QMessageBox::Ok);
  setStyleSheet("QMessageBox QLabel {"
                "    color:" +
                Colors::StandardBlack.name() + ";" +
                TextStyle::BodyMediumRegular() +
                "}"
                "QPushButton {"
                "    background-color: " +
                Colors::Primary500.name() +
                ";"
                "    color: white;"
                "    border: none;"
                "    padding: 6px 12px;"
                "    border-radius: 4px;"
                "}"
                "QPushButton:hover {"
                "    background-color: " +
                Colors::Primary600.name() +
                ";"
                "}");
  exec();
}

QMessageBox::Icon MessageBoxWidget::switchIcon(IconType iconType) {
  switch (iconType) {
  case NoIcon:
    return QMessageBox::NoIcon;
  case Question:
    return QMessageBox::Question;
  case Information:
    return QMessageBox::Information;
  case Warning:
    return QMessageBox::Warning;
  case Critical:
    return QMessageBox::Critical;
  default:
    return QMessageBox::NoIcon;
  }
}