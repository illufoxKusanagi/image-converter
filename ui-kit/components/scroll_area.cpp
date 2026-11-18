#include "scroll_area.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QVariant>

namespace ui {

ScrollArea::ScrollArea(QWidget *parent) : QScrollArea(parent) {
  setFrameShape(QFrame::NoFrame);
  setWidgetResizable(true);
  horizontalScrollBar()->setFocusPolicy(Qt::StrongFocus);
  verticalScrollBar()->setFocusPolicy(Qt::StrongFocus);

  connect(&Theme::instance(), &Theme::themeChanged, this,
          &ScrollArea::applyThemeStyles);
  applyThemeStyles();
}

QString ScrollArea::scrollBarStyleSheet() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &s = theme.spacing();
  const auto &r = theme.radius();

  const QString handleRest =
      StyleHelper::toRgbaString(StyleHelper::withAlphaF(c.mutedForeground, 0.40));
  const QString handleHover =
      StyleHelper::toRgbaString(StyleHelper::withAlphaF(c.mutedForeground, 0.70));
  const QString handleActive = StyleHelper::toHexString(c.ring);
  const QString handleDisabled = StyleHelper::toHexString(c.border);

  return QString(
             "QScrollBar {"
             "  background: transparent;"
             "  border: none;"
             "}"
             "QScrollBar:vertical {"
             "  width: %1px;"
             "  margin: 2px 0px;"
             "}"
             "QScrollBar:horizontal {"
             "  height: %1px;"
             "  margin: 0px 2px;"
             "}"
             "QScrollBar::handle {"
             "  background-color: %2;"
             "  border: none;"
             "  border-radius: %3px;"
             "}"
             "QScrollBar::handle:vertical { min-height: %4px; }"
             "QScrollBar::handle:horizontal { min-width: %4px; }"
             "QScrollBar::handle:hover {"
             "  background-color: %5;"
             "}"
             "QScrollBar::handle:focus, QScrollBar::handle:pressed {"
             "  background-color: %6;"
             "}"
             "QScrollBar::handle:disabled {"
             "  background-color: %7;"
             "}"
             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
             "  height: 0px;"
             "  border: none;"
             "  background: transparent;"
             "}"
             "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
             "  width: 0px;"
             "  border: none;"
             "  background: transparent;"
             "}"
             "QScrollBar::up-arrow, QScrollBar::down-arrow, "
             "QScrollBar::left-arrow, QScrollBar::right-arrow {"
             "  width: 0px;"
             "  height: 0px;"
             "  background: none;"
             "}"
             "QScrollBar::add-page, QScrollBar::sub-page {"
             "  background: transparent;"
             "}")
      .arg(s.space2)
      .arg(handleRest)
      .arg(r.sm)
      .arg(s.space6)
      .arg(handleHover)
      .arg(handleActive)
      .arg(handleDisabled);
}

void ScrollArea::applyScrollBarStyles(QAbstractScrollArea *area) {
  if (!area) {
    return;
  }

  const QString style = scrollBarStyleSheet();
  if (QScrollBar *hBar = area->horizontalScrollBar()) {
    hBar->setStyleSheet(style);
  }
  if (QScrollBar *vBar = area->verticalScrollBar()) {
    vBar->setStyleSheet(style);
  }
}

void ScrollArea::applyThemeStyles() {
  const auto &c = Theme::instance().colors();

  viewport()->setProperty("uiScrollAreaViewport", true);
  setStyleSheet(
      QString("ui--ScrollArea {"
              "  background-color: %1;"
              "  border: none;"
              "}"
              "ui--ScrollArea > QWidget[uiScrollAreaViewport=\"true\"] {"
              "  background-color: %1;"
              "  border: none;"
              "}"
              "ui--ScrollArea::corner {"
              "  background-color: %1;"
              "  border: none;"
              "}")
          .arg(StyleHelper::toHexString(c.background)));

  // Style only our own scrollbars, never scrollbars inside the content widget.
  applyScrollBarStyles(this);
}

} // namespace ui
