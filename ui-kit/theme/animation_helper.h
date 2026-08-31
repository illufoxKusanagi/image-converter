#ifndef UI_ANIMATION_HELPER_H
#define UI_ANIMATION_HELPER_H

#include <QEasingCurve>

namespace ui {

class AnimationHelper {
public:
  static constexpr int DurationInstant = 0;
  static constexpr int DurationFast = 150;
  static constexpr int DurationNormal = 200;
  static constexpr int DurationSlow = 300;

  static QEasingCurve standardEasing() {
    return QEasingCurve(QEasingCurve::OutCubic);
  }

  static QEasingCurve smoothEasing() {
    return QEasingCurve(QEasingCurve::InOutQuad);
  }
};

} // namespace ui

#endif // UI_ANIMATION_HELPER_H
