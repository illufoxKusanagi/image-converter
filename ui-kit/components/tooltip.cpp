#include "tooltip.h"
#include "../theme/style_helper.h"
#include <QApplication>
#include <QToolTip>

namespace ui {

void Tooltip::attach(QWidget *widget, const QString &tooltipText) {
  if (widget) {
    widget->setToolTip(tooltipText);
  }
}

void Tooltip::applyGlobalStyle() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  QToolTip::setFont(t.font(t.sizeXs, QFont::Normal));

  qApp->setStyleSheet(qApp->styleSheet() + QString(
      "QToolTip {"
      "  background-color: %1;"
      "  color: %2;"
      "  border: 1px solid %3;"
      "  border-radius: %4px;"
      "  padding: 4px 8px;"
      "}")
      .arg(StyleHelper::toHexString(c.popover))
      .arg(StyleHelper::toHexString(c.popoverForeground))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.sm));
}

} // namespace ui

