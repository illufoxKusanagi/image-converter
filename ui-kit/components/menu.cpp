#include "menu.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"

#include <QPalette>

namespace ui {

Menu::Menu(QWidget *parent) : Menu(QString(), parent) {}

Menu::Menu(const QString &title, QWidget *parent) : QMenu(title, parent) {
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Menu::applyThemeStyles);
  applyThemeStyles();
}

void Menu::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &r = theme.radius();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();

  setFont(t.font(t.sizeBase));
  QPalette menuPalette = palette();
  menuPalette.setColor(QPalette::Window, c.popover);
  menuPalette.setColor(QPalette::WindowText, c.popoverForeground);
  menuPalette.setColor(QPalette::Text, c.popoverForeground);
  menuPalette.setColor(QPalette::ButtonText, c.popoverForeground);
  menuPalette.setColor(QPalette::Highlight, c.accent);
  menuPalette.setColor(QPalette::HighlightedText, c.accentForeground);
  menuPalette.setColor(QPalette::Disabled, QPalette::WindowText,
                       c.mutedForeground);
  menuPalette.setColor(QPalette::Disabled, QPalette::Text, c.mutedForeground);
  menuPalette.setColor(QPalette::Disabled, QPalette::ButtonText,
                       c.mutedForeground);
  setPalette(menuPalette);

  // This local stylesheet also covers native submenus owned by this menu.
  // Indicators and arrows are deliberately left to the Qt style.
  setStyleSheet(QStringLiteral("QMenu {"
                               "  background-color: %1;"
                               "  color: %2;"
                               "  border: 1px solid %3;"
                               "  border-radius: %4px;"
                               "  padding: %5px;"
                               "}"
                               "QMenu::item {"
                               "  padding: %6px %7px;"
                               "  border-radius: %8px;"
                               "}"
                               "QMenu::item:selected {"
                               "  background-color: %9;"
                               "  color: %10;"
                               "}"
                               "QMenu::item:disabled {"
                               "  color: %11;"
                               "}"
                               "QMenu::separator {"
                               "  height: 1px;"
                               "  background-color: %3;"
                               "  margin: %5px %6px;"
                               "}")
                    .arg(StyleHelper::toHexString(c.popover))
                    .arg(StyleHelper::toHexString(c.popoverForeground))
                    .arg(StyleHelper::toHexString(c.border))
                    .arg(r.md)
                    .arg(s.space1)
                    .arg(s.space2)
                    .arg(s.space6)
                    .arg(r.sm)
                    .arg(StyleHelper::toHexString(c.accent))
                    .arg(StyleHelper::toHexString(c.accentForeground))
                    .arg(StyleHelper::toHexString(c.mutedForeground)));
}

} // namespace ui
