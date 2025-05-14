#include "button_action.h"

/**
 * @brief Constructs a customized button with optional icon based on its intended action.
 *
 * Initializes the button with the specified text and parent widget. Sets a right-arrow icon if the button is a "run" button, or an upload icon if specified. No icon is set for other values.
 *
 * @param parent The parent widget.
 * @param buttonText The text to display on the button.
 * @param isRunButton Determines the icon: "yes" for a run icon, "upload" for an upload icon, or no icon otherwise.
 */
ButtonAction::ButtonAction(QWidget *parent, const QString &buttonText,
                           const QString &isRunButton)
    : QPushButton(buttonText, parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  if (isRunButton == "yes") {
    setIcon(QIcon(":/icons/icons/right-arrow-active.svg"));
    setIconSize(QSize(24, 24));
  } else if (isRunButton == "upload") {
    setIcon(QIcon(":/icons/icons/upload.svg"));
    setIconSize(QSize(24, 24));
  }
}

QString ButtonAction::getText() const { return text(); }

/**
 * @brief Enables or disables the button and updates its visual style accordingly.
 *
 * When enabled, applies a primary color scheme with hover and pressed effects. When disabled, applies a greyed-out style.
 *
 * @param isEnabled If true, the button is enabled and styled as active; if false, it is disabled and styled as inactive.
 */
void ButtonAction::setEnabled(bool isEnabled) {
  QPushButton::setEnabled(isEnabled);
  if (isEnabled) {
    setStyleSheet("QPushButton {"
                  "    padding: 12px 4px;"
                  "    border: 0 solid;"
                  "    border-radius: 12px;"
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