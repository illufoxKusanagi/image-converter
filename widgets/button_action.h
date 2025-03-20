#ifndef BUTTONACTION_H
#define BUTTONACTION_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QHBoxLayout>
#include <QPushButton>

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
