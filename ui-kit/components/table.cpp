#include "table.h"
#include "../theme/style_helper.h"
#include <QHeaderView>

namespace ui {

Table::Table(QWidget *parent) : Table(0, 0, parent) {}

Table::Table(int rows, int columns, QWidget *parent)
    : QTableWidget(rows, columns, parent) {
  setAlternatingRowColors(false);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setShowGrid(false);
  verticalHeader()->setVisible(false);
  horizontalHeader()->setStretchLastSection(true);
  horizontalHeader()->setHighlightSections(false);

  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Table::applyThemeStyles);
  applyThemeStyles();
}

void Table::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  setFont(t.font(t.sizeSm, QFont::Normal));
  horizontalHeader()->setFont(t.font(t.sizeXs, QFont::DemiBold));

  setStyleSheet(
      QString("QTableWidget {"
              "  background-color: %1;"
              "  color: %2;"
              "  border: 1px solid %3;"
              "  border-radius: %4px;"
              "  gridline-color: %3;"
              "  outline: none;"
              "}"
              "QTableWidget::item {"
              "  padding: 6px 10px;"
              "  border-bottom: 1px solid %3;"
              "}"
              "QTableWidget::item:selected {"
              "  background-color: %5;"
              "  color: %6;"
              "}"
              "QHeaderView::section {"
              "  background-color: %7;"
              "  color: %8;"
              "  padding: 6px 10px;"
              "  border: none;"
              "  border-bottom: 1px solid %3;"
              "}")
          .arg(StyleHelper::toHexString(c.card))
          .arg(StyleHelper::toHexString(c.foreground))
          .arg(StyleHelper::toHexString(c.border))
          .arg(r.md)
          .arg(StyleHelper::toHexString(c.accent))
          .arg(StyleHelper::toHexString(c.accentForeground))
          .arg(StyleHelper::toHexString(c.muted))
          .arg(StyleHelper::toHexString(c.mutedForeground)));
}

} // namespace ui

