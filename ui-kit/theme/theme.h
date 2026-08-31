#ifndef UI_THEME_H
#define UI_THEME_H

#include "tokens.h"
#include <QObject>

namespace ui {

class Theme : public QObject {
  Q_OBJECT

public:
  static Theme &instance();

  ThemeMode mode() const;
  bool isDark() const;
  bool isSystemDark() const;

  void setMode(ThemeMode mode);
  void toggleMode();

  const ColorTokens &colors() const;
  const RadiusTokens &radius() const;
  const SpacingTokens &spacing() const;
  const TypographyTokens &typography() const;

  void setPrimaryColor(const QColor &primary, const QColor &primaryForeground = QColor());
  void setFontFamily(const QString &fontFamily);

signals:
  void themeChanged(ui::ThemeMode mode);

private:
  explicit Theme(QObject *parent = nullptr);
  ~Theme() override = default;
  Theme(const Theme &) = delete;
  Theme &operator=(const Theme &) = delete;

  ThemeMode m_mode{ThemeMode::System};
  ColorTokens m_lightColors{lightColorTokens()};
  ColorTokens m_darkColors{darkColorTokens()};
  RadiusTokens m_radius{};
  SpacingTokens m_spacing{};
  TypographyTokens m_typography{};
};

} // namespace ui

#endif // UI_THEME_H
