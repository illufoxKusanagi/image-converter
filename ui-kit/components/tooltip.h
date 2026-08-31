#ifndef UI_TOOLTIP_H
#define UI_TOOLTIP_H

#include "../theme/theme.h"
#include <QString>
#include <QWidget>

namespace ui {

class Tooltip {
public:
  static void attach(QWidget *widget, const QString &tooltipText);
  static void applyGlobalStyle();
};

} // namespace ui

#endif // UI_TOOLTIP_H

