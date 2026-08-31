#include "select.h"
#include "../theme/style_helper.h"
#include <QListView>

namespace ui {

Select::Select(QWidget *parent)
    : Select(QStringList(), parent) {}

Select::Select(const QStringList &items, QWidget *parent)
    : QComboBox(parent) {
  setFixedHeight(40);
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

  setFont(t.font(t.sizeBase, QFont::Normal));

  setStyleSheet(QString(
      "QComboBox {"
      "  background-color: %1;"
      "  color: %2;"
      "  border: 1px solid %3;"
      "  border-radius: %4px;"
      "  padding: 0px 32px 0px 12px;"
      "  min-height: 38px;"
      "}"
      "QComboBox:hover {"
      "  border-color: %5;"
      "}"
      "QComboBox:focus {"
      "  border: 2px solid %6;"
      "}"
      "QComboBox::drop-down {"
      "  subcontrol-origin: padding;"
      "  subcontrol-position: top right;"
      "  width: 28px;"
      "  border-left: none;"
      "}"
      "QComboBox::down-arrow {"
      "  image: url(:/icons/icons/lucide-chevron-down.svg);"
      "  width: 14px;"
      "  height: 14px;"
      "  margin-right: 6px;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background-color: %7;"
      "  color: %8;"
      "  border: 1px solid %9;"
      "  border-radius: %10px;"
      "  padding: 4px;"
      "  selection-background-color: %11;"
      "  selection-color: %12;"
      "  outline: none;"
      "}"
      "QComboBox QAbstractItemView::item {"
      "  min-height: 32px;"
      "  padding: 4px 8px;"
      "  border-radius: 4px;"
      "}"
      "QComboBox QAbstractItemView::item:hover {"
      "  background-color: %13;"
      "  color: %14;"
      "}")
      .arg(StyleHelper::toHexString(c.background))
      .arg(StyleHelper::toHexString(c.foreground))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.md)
      .arg(StyleHelper::toHexString(c.primary))
      .arg(StyleHelper::toHexString(c.ring))
      .arg(StyleHelper::toHexString(c.popover))
      .arg(StyleHelper::toHexString(c.popoverForeground))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.md)
      .arg(StyleHelper::toHexString(c.accent))
      .arg(StyleHelper::toHexString(c.accentForeground))
      .arg(StyleHelper::toHexString(c.accent))
      .arg(StyleHelper::toHexString(c.accentForeground)));
}

} // namespace ui
