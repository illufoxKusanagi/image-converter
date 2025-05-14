#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QMessageBox>

class MessageBoxWidget : public QMessageBox {
  Q_OBJECT

public:
  enum IconType {
    NoIcon = 0,
    Question = 1,
    Information = 2,
    Warning = 3,
    Critical = 4
  };
  explicit MessageBoxWidget(QString windowTitle, QString text,
                            IconType iconType);

private:
  QMessageBox::Icon switchIcon(IconType iconType);
};

#endif // MESSAGE_BOX_H
