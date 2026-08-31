#include "theme.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <algorithm>
#include <cmath>

namespace ui {

Theme &Theme::instance() {
  static Theme s_instance;
  return s_instance;
}

Theme::Theme(QObject *parent)
    : QObject(parent),
      m_lightColors(lightColorTokens()),
      m_darkColors(darkColorTokens()),
      m_radius(RadiusTokens{}),
      m_spacing(SpacingTokens{}),
      m_typography(TypographyTokens{}) {
  if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
    if (auto *hints = app->styleHints()) {
      connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
        if (m_mode == ThemeMode::System) {
          emit themeChanged(m_mode);
        }
      });
    }
  }
}

ThemeMode Theme::mode() const {
  return m_mode;
}

bool Theme::isSystemDark() const {
  if (auto *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
    if (auto *hints = app->styleHints()) {
      return hints->colorScheme() == Qt::ColorScheme::Dark;
    }
  }
  return false;
}

void Theme::setMode(ThemeMode mode) {
  if (m_mode != mode) {
    m_mode = mode;
    emit themeChanged(m_mode);
  }
}

void Theme::toggleMode() {
  if (m_mode == ThemeMode::Light) {
    setMode(ThemeMode::Dark);
  } else if (m_mode == ThemeMode::Dark) {
    setMode(ThemeMode::System);
  } else {
    setMode(ThemeMode::Light);
  }
}

bool Theme::isDark() const {
  if (m_mode == ThemeMode::System) {
    return isSystemDark();
  }
  return m_mode == ThemeMode::Dark;
}

const ColorTokens &Theme::colors() const {
  return isDark() ? m_darkColors : m_lightColors;
}

const RadiusTokens &Theme::radius() const {
  return m_radius;
}

const SpacingTokens &Theme::spacing() const {
  return m_spacing;
}

const TypographyTokens &Theme::typography() const {
  return m_typography;
}

void Theme::setPrimaryColor(const QColor &primary, const QColor &primaryForeground) {
  m_lightColors.primary = primary;
  m_darkColors.primary = primary;
  m_lightColors.ring = primary;
  m_darkColors.ring = primary;

  if (primaryForeground.isValid()) {
    m_lightColors.primaryForeground = primaryForeground;
    m_darkColors.primaryForeground = primaryForeground;
  } else {
    auto luminance = [](const QColor &color) {
      double r = color.redF();
      double g = color.greenF();
      double b = color.blueF();
      auto adjust = [](double c) {
        return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
      };
      return 0.2126 * adjust(r) + 0.7152 * adjust(g) + 0.0722 * adjust(b);
    };

    double lum = luminance(primary);
    double contrastWithWhite = 1.05 / (lum + 0.05);
    double contrastWithBlack = (lum + 0.05) / 0.051;
    QColor contrastFg = contrastWithBlack > contrastWithWhite ? QColor("#09090b") : QColor("#ffffff");
    m_lightColors.primaryForeground = contrastFg;
    m_darkColors.primaryForeground = contrastFg;
  }

  emit themeChanged(m_mode);
}

void Theme::setFontFamily(const QString &fontFamily) {
  m_typography.fontFamily = fontFamily;
  emit themeChanged(m_mode);
}

} // namespace ui
