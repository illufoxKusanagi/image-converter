#ifndef BUTTONTOGGLE_H
#define BUTTONTOGGLE_H

#include "styles/colors.h"
#include <QPushButton>
#include <QWidget>

class ButtonToggle : public QPushButton {
  Q_OBJECT
public:
  explicit ButtonToggle(bool isCollapsed = false, QWidget *parent = nullptr);
  void updateIcon();
  void toggleCollapse();

private:
  bool m_isCollapsed = false;
};

#endif // ButtonToggle_H
