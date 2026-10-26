#include "select.h"
#include "../theme/style_helper.h"
#include <QListView>

namespace ui {

Select::Select(QWidget *parent)
    : Select(QStringList(), parent) {}

Select::Select(const QStringList &items, QWidget *parent)
    : QComboBox(parent) {
  setFixedHeight(38);
  setCursor(Qt::PointingHandCursor);
  setView(new QListView(this));

  if (!items.isEmpty()) {
    addItems(items);
  }

  connect(&Theme::instance(), &Theme::themeChanged, this, &Select::applyThemeStyles);
  applyThemeStyles();
}

void Select::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();
  bool isDark = Theme::instance().isDark();

  setFont(t.font(t.sizeBase, QFont::Normal));
  if (view()) {
    view()->setFont(t.font(t.sizeBase, QFont::Normal));
  }

  QString bgHex = StyleHelper::toHexString(c.background);
  QString fgHex = StyleHelper::toHexString(c.foreground);
  QString borderHex = StyleHelper::toHexString(c.primary);
  QString hoverBorderHex = StyleHelper::toHexString(StyleHelper::hoverColor(c.primary, isDark));
  QString popoverHex = StyleHelper::toHexString(c.popover);
  QString popoverFgHex = StyleHelper::toHexString(c.popoverForeground);
  QString accentHex = StyleHelper::toHexString(c.accent);
  QString accentFgHex = StyleHelper::toHexString(c.accentForeground);
  QColor compBg = isDark ? c.secondary : StyleHelper::tint(c.background, c.primary, 0.12);
  QColor compHover = isDark ? StyleHelper::lighten(c.secondary, 18) : StyleHelper::tint(c.background, c.primary, 0.22);
  QString compartmentBgHex = StyleHelper::toHexString(compBg);
  QString compartmentHoverHex = StyleHelper::toHexString(compHover);
  QString downIcon = isDark ? ":/icons/icons/arrow-down.svg" : ":/icons/icons/arrow-down-black.svg";

  setStyleSheet(
      "QComboBox {"
      "  background-color: " + bgHex + ";"
      "  color: " + fgHex + ";"
      "  border: 1px solid " + borderHex + ";"
      "  border-radius: " + QString::number(r.md) + "px;"
      "  padding: 0px 36px 0px 12px;"
      "  font-size: " + QString::number(t.sizeBase) + "px;"
      "  font-family: '" + t.fontFamily + "';"
      "}"
      "QComboBox:hover, QComboBox:focus {"
      "  border-color: " + hoverBorderHex + ";"
      "}"
      "QComboBox::drop-down {"
      "  subcontrol-origin: border;"
      "  subcontrol-position: top right;"
      "  width: 32px;"
      "  border: 1px solid " + borderHex + ";"
      "  border-top-right-radius: " + QString::number(r.md - 1) + "px;"
      "  border-bottom-right-radius: " + QString::number(r.md - 1) + "px;"
      "  background-color: " + compartmentBgHex + ";"
      "}"
      "QComboBox:hover::drop-down, QComboBox:focus::drop-down {"
      "  border-color: " + hoverBorderHex + ";"
      "}"
      "QComboBox::drop-down:hover {"
      "  background-color: " + compartmentHoverHex + ";"
      "}"
      "QComboBox::down-arrow {"
      "  image: url(" + downIcon + ");"
      "  width: 10px;"
      "  height: 10px;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background-color: " + popoverHex + ";"
      "  color: " + popoverFgHex + ";"
      "  border: 1px solid " + borderHex + ";"
      "  border-radius: 8px;"
      "  padding: 4px;"
      "  outline: none;"
      "  font-size: " + QString::number(t.sizeBase) + "px;"
      "}"
      "QComboBox QAbstractItemView::item {"
      "  min-height: 30px;"
      "  padding: 6px 12px;"
      "  margin: 2px 4px;"
      "  border-radius: 6px;"
      "  color: " + popoverFgHex + ";"
      "}"
      "QComboBox QAbstractItemView::item:hover, QComboBox QAbstractItemView::item:selected {"
      "  background-color: " + accentHex + ";"
      "  color: " + accentFgHex + ";"
      "  border-radius: 6px;"
      "}");
}

} // namespace ui
