#include "button_action.h"

ButtonAction::ButtonAction(const QString &buttonText,
                           const QString &isRunButton, QWidget *parent)
    : QPushButton(buttonText, parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  if (isRunButton == "yes") {
    setIcon(QIcon(":/icons/right-arrow-active.svg"));
    setIconSize(QSize(24, 24));
  }
}

QString ButtonAction::getText() const { return text(); }

void ButtonAction::setEnabled(bool isEnabled) {
  QPushButton::setEnabled(isEnabled);
  if (isEnabled) {
    setStyleSheet("QPushButton {"
                  "    padding: 12px 4px;"
                  "    border: 0 solid;"
                  "    border-radius: 12px;"
                  "    max-width: 150px;"
                  "    background-color: " +
                  Colors::Primary600.name() +
                  ";"
                  "    color: " +
                  Colors::StandardWhite.name() +
                  ";"
                  "    " +
                  TextStyle::BodyMediumBold() +
                  "}"
                  "QPushButton:hover {"
                  "    background-color: " +
                  Colors::Primary400.name() +
                  ";"
                  "}"
                  "QPushButton:pressed {"
                  "    background-color: " +
                  Colors::Primary500.name() +
                  ";"
                  "}");
  } else {
    setStyleSheet("QPushButton {"
                  "    padding: 12px 4px;"
                  "    border: 0 solid;"
                  "    border-radius: 12px;"
                  "    max-width: 150px;"
                  "    background-color: " +
                  Colors::Grey400.name() +
                  ";"
                  "    color: " +
                  Colors::StandardWhite.name() +
                  ";"
                  "    " +
                  TextStyle::BodyMediumBold() + "}");
  }
}

void ButtonAction::setSize(int width, int height) {
  setFixedSize(width, height);
}