#ifndef UI_STYLE_HELPER_H
#define UI_STYLE_HELPER_H

#include <QColor>
#include <QString>

namespace ui {

class StyleHelper {
public:
  static QString toRgbaString(const QColor &c) {
    return QString("rgba(%1, %2, %3, %4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alphaF(), 0, 'f', 2);
  }

  static QString toHexString(const QColor &c) {
    return c.name(QColor::HexRgb);
  }

  static QColor withAlpha(const QColor &c, int alpha) {
    QColor result = c;
    result.setAlpha(qBound(0, alpha, 255));
    return result;
  }

  static QColor withAlphaF(const QColor &c, qreal alphaF) {
    QColor result = c;
    result.setAlphaF(qBound(0.0, alphaF, 1.0));
    return result;
  }

  static QColor lighten(const QColor &c, int amount = 10) {
    return c.lighter(100 + amount);
  }

  static QColor darken(const QColor &c, int amount = 10) {
    return c.darker(100 + amount);
  }

  static QColor hoverColor(const QColor &c, bool isDark) {
    return isDark ? c.lighter(120) : c.darker(108);
  }

  static QColor tint(const QColor &surface, const QColor &accent, qreal accentRatio) {
    qreal ratio = qBound(0.0, accentRatio, 1.0);
    int r = qRound(surface.red() * (1.0 - ratio) + accent.red() * ratio);
    int g = qRound(surface.green() * (1.0 - ratio) + accent.green() * ratio);
    int b = qRound(surface.blue() * (1.0 - ratio) + accent.blue() * ratio);
    return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
  }
};

} // namespace ui

#endif // UI_STYLE_HELPER_H
