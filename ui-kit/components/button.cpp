#include "button.h"
#include "../theme/style_helper.h"

namespace ui {

Button::Button(QWidget *parent)
    : Button("", ButtonVariant::Primary, ButtonSize::Default, parent) {}

Button::Button(const QString &text, QWidget *parent)
    : Button(text, ButtonVariant::Primary, ButtonSize::Default, parent) {}

Button::Button(const QString &text, ButtonVariant variant, ButtonSize size, QWidget *parent)
    : QPushButton(text, parent), m_variant(variant), m_size(size) {
  setCursor(Qt::PointingHandCursor);
  connect(&Theme::instance(), &Theme::themeChanged, this, &Button::applyThemeStyles);
  applyThemeStyles();
}

ButtonVariant Button::variant() const {
  return m_variant;
}

void Button::setVariant(ButtonVariant variant) {
  if (m_variant != variant) {
    m_variant = variant;
    applyThemeStyles();
  }
}

ButtonSize Button::size() const {
  return m_size;
}

void Button::setSize(ButtonSize size) {
  if (m_size != size) {
    m_size = size;
    applyThemeStyles();
  }
}

bool Button::isLoading() const {
  return m_isLoading;
}

void Button::setLoading(bool loading) {
  if (m_isLoading == loading) {
    return;
  }
  m_isLoading = loading;
  setEnabled(!loading);
  if (loading) {
    m_savedText = text();
    setText("Please wait...");
  } else {
    setText(m_savedText);
  }
  applyThemeStyles();
}

void Button::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();
  bool isDark = Theme::instance().isDark();

  int height = 40;
  int paddingH = 16;
  int fontSize = t.sizeBase;
  int borderRadius = r.md;

  switch (m_size) {
  case ButtonSize::Small:
    height = 32;
    paddingH = 12;
    fontSize = t.sizeSm;
    borderRadius = r.sm;
    break;
  case ButtonSize::Large:
    height = 48;
    paddingH = 24;
    fontSize = t.sizeLg;
    borderRadius = r.lg;
    break;
  case ButtonSize::Icon:
    height = 36;
    paddingH = 0;
    setFixedWidth(36);
    break;
  case ButtonSize::Default:
  default:
    height = 40;
    paddingH = 16;
    fontSize = t.sizeBase;
    borderRadius = r.md;
    break;
  }

  if (m_size != ButtonSize::Icon) {
    setMinimumWidth(0);
    setMaximumWidth(16777215);
  }
  setFixedHeight(height);
  setFont(t.font(fontSize, QFont::DemiBold));

  QString bg, fg, hoverBg, pressedBg, border, hoverBorder;

  switch (m_variant) {
  case ButtonVariant::Secondary:
    bg = StyleHelper::toHexString(c.secondary);
    fg = StyleHelper::toHexString(c.secondaryForeground);
    hoverBg = StyleHelper::toHexString(StyleHelper::hoverColor(c.secondary, isDark));
    pressedBg = StyleHelper::toHexString(StyleHelper::darken(c.secondary, 10));
    border = "transparent";
    hoverBorder = "transparent";
    break;

  case ButtonVariant::Outline:
    bg = "transparent";
    fg = StyleHelper::toHexString(c.foreground);
    hoverBg = StyleHelper::toHexString(c.accent);
    pressedBg = StyleHelper::toHexString(StyleHelper::hoverColor(c.accent, isDark));
    border = StyleHelper::toHexString(c.border);
    hoverBorder = StyleHelper::toHexString(c.border);
    break;

  case ButtonVariant::Ghost:
    bg = "transparent";
    fg = StyleHelper::toHexString(c.foreground);
    hoverBg = StyleHelper::toHexString(c.accent);
    pressedBg = StyleHelper::toHexString(StyleHelper::hoverColor(c.accent, isDark));
    border = "transparent";
    hoverBorder = "transparent";
    break;

  case ButtonVariant::Destructive:
    bg = StyleHelper::toHexString(c.destructive);
    fg = StyleHelper::toHexString(c.destructiveForeground);
    hoverBg = StyleHelper::toHexString(StyleHelper::lighten(c.destructive, 10));
    pressedBg = StyleHelper::toHexString(StyleHelper::darken(c.destructive, 10));
    border = "transparent";
    hoverBorder = "transparent";
    break;

  case ButtonVariant::Link:
    bg = "transparent";
    fg = StyleHelper::toHexString(c.primary);
    hoverBg = "transparent";
    pressedBg = "transparent";
    border = "transparent";
    hoverBorder = "transparent";
    break;

  case ButtonVariant::Primary:
  default:
    bg = StyleHelper::toHexString(c.primary);
    fg = StyleHelper::toHexString(c.primaryForeground);
    hoverBg = StyleHelper::toHexString(StyleHelper::withAlphaF(c.primary, 0.9));
    pressedBg = StyleHelper::toHexString(StyleHelper::darken(c.primary, 15));
    border = "transparent";
    hoverBorder = "transparent";
    break;
  }

  QString style = QString(
      "QPushButton {"
      "  background-color: %1;"
      "  color: %2;"
      "  border: 1px solid %3;"
      "  border-radius: %4px;"
      "  padding: 0px %5px;"
      "  text-align: center;"
      "}"
      "QPushButton:hover {"
      "  background-color: %6;"
      "  border-color: %7;"
      "}"
      "QPushButton:pressed {"
      "  background-color: %8;"
      "}"
      "QPushButton:focus {"
      "  outline: none;"
      "  border: 2px solid %9;"
      "}"
      "QPushButton:disabled {"
      "  background-color: %10;"
      "  color: %11;"
      "  border-color: transparent;"
      "}")
      .arg(bg)
      .arg(fg)
      .arg(border)
      .arg(borderRadius)
      .arg(paddingH)
      .arg(hoverBg)
      .arg(hoverBorder)
      .arg(pressedBg)
      .arg(StyleHelper::toHexString(c.ring))
      .arg(StyleHelper::toHexString(StyleHelper::withAlphaF(c.muted, 0.5)))
      .arg(StyleHelper::toHexString(c.mutedForeground));

  if (m_variant == ButtonVariant::Link) {
    style += "QPushButton:hover { text-decoration: underline; }";
  }

  setStyleSheet(style);
}

} // namespace ui
