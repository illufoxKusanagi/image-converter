#include "scroll_area.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
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

void ScrollArea::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &s = theme.spacing();
  const auto &r = theme.radius();

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

  const QString scrollBarStyle =
      QString(
          "QScrollBar {"
          "  background-color: %1;"
          "  border: none;"
          "  margin: %2px;"
          "}"
          "QScrollBar:vertical { width: %3px; }"
          "QScrollBar:horizontal { height: %3px; }"
          "QScrollBar::handle {"
          "  background-color: %4;"
          "  border: 1px solid %4;"
          "  border-radius: %5px;"
          "}"
          "QScrollBar::handle:vertical { min-height: %6px; }"
          "QScrollBar::handle:horizontal { min-width: %6px; }"
          "QScrollBar::handle:hover {"
          "  background-color: %7;"
          "  border-color: %7;"
          "}"
          "QScrollBar::handle:focus { border-color: %8; }"
          "QScrollBar::handle:pressed {"
          "  background-color: %8;"
          "  border-color: %8;"
          "}"
          "QScrollBar::handle:disabled {"
          "  background-color: %9;"
          "  border-color: %9;"
          "}"
          "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
          "  height: %2px;"
          "  border: none;"
          "  background: transparent;"
          "}"
          "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
          "  width: %2px;"
          "  border: none;"
          "  background: transparent;"
          "}"
          "QScrollBar::add-page, QScrollBar::sub-page {"
          "  background: transparent;"
          "}")
          .arg(StyleHelper::toHexString(c.muted))
          .arg(s.space0)
          .arg(s.space3)
          .arg(StyleHelper::toHexString(c.mutedForeground))
          .arg(r.sm)
          .arg(s.space6)
          .arg(StyleHelper::toHexString(c.foreground))
          .arg(StyleHelper::toHexString(c.ring))
          .arg(StyleHelper::toHexString(c.border));

  // Style only our own scrollbars, never scrollbars inside the content widget.
  horizontalScrollBar()->setStyleSheet(scrollBarStyle);
  verticalScrollBar()->setStyleSheet(scrollBarStyle);
}

} // namespace ui
