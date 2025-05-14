#ifndef BUTTONACTION_H
#define BUTTONACTION_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QHBoxLayout>
#include <QPushButton>

/**
 * @brief Creates a custom button with optional text and run-action designation.
 *
 * Constructs a ButtonAction widget, optionally setting its parent, display text, and a string indicating if it is a "run" button.
 *
 * @param parent Optional parent widget.
 * @param buttonText Optional text to display on the button.
 * @param isRunButton Optional string to indicate if the button is a "run" button.
 */
class ButtonAction : public QPushButton {
  Q_OBJECT

public:
  explicit ButtonAction(QWidget *parent = nullptr,
                        const QString &buttonText = "",
                        const QString &isRunButton = "");
  using QPushButton::setText;
  QString getText() const;
  void setEnabled(bool isEnabled);
  void setSize(int width, int height);

private:
  QPushButton *buttonAction;
};

#endif // BUTTONACTION_H
