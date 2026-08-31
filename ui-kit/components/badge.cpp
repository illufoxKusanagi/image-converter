#include "badge.h"
#include "../theme/style_helper.h"

namespace ui {

Badge::Badge(QWidget *parent)
    : Badge("", BadgeVariant::Default, parent) {}

Badge::Badge(const QString &text, QWidget *parent)
    : Badge(text, BadgeVariant::Default, parent) {}

Badge::Badge(const QString &text, BadgeVariant variant, QWidget *parent)
    : QLabel(text, parent), m_variant(variant) {
  setAlignment(Qt::AlignCenter);
  connect(&Theme::instance(), &Theme::themeChanged, this, &Badge::applyThemeStyles);
  applyThemeStyles();
}

BadgeVariant Badge::variant() const {
  return m_variant;
}

void Badge::setVariant(BadgeVariant variant) {
  if (m_variant != variant) {
    m_variant = variant;
    applyThemeStyles();
  }
}

void Badge::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &t = Theme::instance().typography();

  setFont(t.font(t.sizeXs, QFont::DemiBold));

  QString bg, fg, border;

  switch (m_variant) {
  case BadgeVariant::Secondary:
    bg = StyleHelper::toHexString(c.secondary);
    fg = StyleHelper::toHexString(c.secondaryForeground);
    border = "transparent";
    break;

  case BadgeVariant::Outline:
    bg = "transparent";
    fg = StyleHelper::toHexString(c.foreground);
    border = StyleHelper::toHexString(c.border);
    break;

  case BadgeVariant::Destructive:
    bg = StyleHelper::toHexString(c.destructive);
    fg = StyleHelper::toHexString(c.destructiveForeground);
    border = "transparent";
    break;

  case BadgeVariant::Default:
  default:
    bg = StyleHelper::toHexString(c.primary);
    fg = StyleHelper::toHexString(c.primaryForeground);
    border = "transparent";
    break;
  }

  setStyleSheet(QString(
      "QLabel {"
      "  background-color: %1;"
      "  color: %2;"
      "  border: 1px solid %3;"
      "  border-radius: 9999px;"
      "  padding: 2px 8px;"
      "}")
      .arg(bg)
      .arg(fg)
      .arg(border));
}

} // namespace ui
