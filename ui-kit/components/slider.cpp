#include "slider.h"
#include "../theme/style_helper.h"

namespace ui {

Slider::Slider(QWidget *parent)
    : Slider(Qt::Horizontal, parent) {}

Slider::Slider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent) {
  setCursor(Qt::PointingHandCursor);
  connect(&Theme::instance(), &Theme::themeChanged, this, &Slider::applyThemeStyles);
  applyThemeStyles();
}

void Slider::applyThemeStyles() {
  const auto &c = Theme::instance().colors();

  setStyleSheet(QString(
      "QSlider {"
      "  min-height: 24px;"
      "}"
      "QSlider::groove:horizontal {"
      "  height: 6px;"
      "  background: %1;"
      "  border-radius: 3px;"
      "}"
      "QSlider::sub-page:horizontal {"
      "  height: 6px;"
      "  background: %2;"
      "  border-radius: 3px;"
      "}"
      "QSlider::handle:horizontal {"
      "  background: %3;"
      "  border: 2px solid %4;"
      "  width: 18px;"
      "  height: 18px;"
      "  margin: -6px 0px;"
      "  border-radius: 9px;"
      "}"
      "QSlider::handle:horizontal:hover {"
      "  background: %5;"
      "  border-color: %6;"
      "}"
      "QSlider::handle:horizontal:focus {"
      "  border: 2px solid %7;"
      "}"
      "QSlider::disabled {"
      "  opacity: 0.5;"
      "}")
      .arg(StyleHelper::toHexString(c.secondary))
      .arg(StyleHelper::toHexString(c.primary))
      .arg(StyleHelper::toHexString(c.background))
      .arg(StyleHelper::toHexString(c.primary))
      .arg(StyleHelper::toHexString(c.accent))
      .arg(StyleHelper::toHexString(c.primary))
      .arg(StyleHelper::toHexString(c.ring)));
}

} // namespace ui
