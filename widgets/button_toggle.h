#ifndef BUTTONTOGGLE_H
#define BUTTONTOGGLE_H

#include "styles/colors.h"
#include <QPushButton>
#include <QWidget>

/**
 * @brief A QPushButton subclass that toggles between collapsed and expanded states.
 *
 * The ButtonToggle provides a button that can represent and switch between collapsed and expanded states, updating its icon accordingly.
 *
 * @param parent Optional parent widget.
 * @param isCollapsed Initial collapsed state of the button.
 */
class ButtonToggle : public QPushButton {
  Q_OBJECT
public:
  explicit ButtonToggle(QWidget *parent = nullptr, bool isCollapsed = false);
  void updateIcon();
  void toggleCollapse();

private:
  bool m_isCollapsed = false;
};

#endif // ButtonToggle_H
