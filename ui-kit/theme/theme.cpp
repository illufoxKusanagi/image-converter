#include "theme.h"

namespace ui {

Theme::Theme(QObject *parent) : QObject(parent) {}

Theme &Theme::instance() {
  static Theme s_instance;
  return s_instance;
}

ThemeMode Theme::mode() const {
  return m_mode;
}

bool Theme::isDark() const {
  return m_mode == ThemeMode::Dark;
}

void Theme::setMode(ThemeMode mode) {
  if (m_mode == mode) {
    return;
  }
  m_mode = mode;
  emit themeChanged(m_mode);
}

void Theme::toggleMode() {
  setMode(m_mode == ThemeMode::Light ? ThemeMode::Dark : ThemeMode::Light);
}

const ColorTokens &Theme::colors() const {
  return (m_mode == ThemeMode::Dark) ? m_darkColors : m_lightColors;
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
    // Automatically calculate optimal contrast foreground color
    double luminance = (0.299 * primary.red() + 0.587 * primary.green() + 0.114 * primary.blue()) / 255.0;
    QColor contrastFg = (luminance > 0.55) ? QColor("#09090b") : QColor("#fafafa");
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

