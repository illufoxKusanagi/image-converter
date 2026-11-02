#include "checkbox.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOptionButton>

namespace ui {

Checkbox::Checkbox(QWidget *parent) : Checkbox(QString(), parent) {}

Checkbox::Checkbox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent) {
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Checkbox::applyThemeStyles);
  applyThemeStyles();
}

bool Checkbox::hasError() const { return m_hasError; }

void Checkbox::setError(bool hasError) {
  if (m_hasError == hasError) {
    return;
  }
  m_hasError = hasError;
  applyThemeStyles();
}

void Checkbox::paintEvent(QPaintEvent *event) {
  QCheckBox::paintEvent(event);
  if (checkState() == Qt::Unchecked) {
    return;
  }

  QStyleOptionButton option;
  initStyleOption(&option);
  const QRect indicator =
      style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, this);
  const qreal inset = Theme::instance().spacing().space1;
  const QRectF mark = QRectF(indicator).adjusted(inset, inset, -inset, -inset);
  const auto &c = Theme::instance().colors();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(isEnabled() ? c.primaryForeground : c.mutedForeground, 2,
                      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  if (checkState() == Qt::PartiallyChecked) {
    painter.drawLine(QPointF(mark.left(), mark.center().y()),
                     QPointF(mark.right(), mark.center().y()));
  } else {
    QPainterPath path;
    path.moveTo(mark.left(), mark.center().y());
    path.lineTo(mark.left() + mark.width() * 0.4, mark.bottom());
    path.lineTo(mark.right(), mark.top());
    painter.drawPath(path);
  }
}

void Checkbox::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &r = theme.radius();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();
  const QColor border =
      m_hasError ? c.destructive
                 : StyleHelper::tint(c.border, c.foreground, 0.35);
  const QColor checkedBorder = m_hasError ? c.destructive : c.primary;
  const QColor focusBorder = m_hasError ? c.destructive : c.ring;

  setFont(t.font(t.sizeBase));
  setStyleSheet(QString("ui--Checkbox {"
                        "  color: %1;"
                        "  spacing: %2px;"
                        "}"
                        "ui--Checkbox::indicator {"
                        "  width: %3px;"
                        "  height: %3px;"
                        "  border: 1px solid %4;"
                        "  border-radius: %5px;"
                        "  background-color: %6;"
                        "  image: none;"
                        "}"
                        "ui--Checkbox::indicator:checked,"
                        "ui--Checkbox::indicator:indeterminate {"
                        "  background-color: %7;"
                        "  border-color: %8;"
                        "}"
                        "ui--Checkbox::indicator:hover { border-color: %8; }"
                        "ui--Checkbox::indicator:focus { border-color: %9; }"
                        "ui--Checkbox:disabled { color: %10; }"
                        "ui--Checkbox::indicator:disabled {"
                        "  background-color: %11;"
                        "  border-color: %12;"
                        "}")
                    .arg(StyleHelper::toHexString(c.foreground))
                    .arg(s.space2)
                    .arg(s.space4 - 2)
                    .arg(StyleHelper::toHexString(border))
                    .arg(r.sm)
                    .arg(StyleHelper::toHexString(c.background))
                    .arg(StyleHelper::toHexString(c.primary))
                    .arg(StyleHelper::toHexString(checkedBorder))
                    .arg(StyleHelper::toHexString(focusBorder))
                    .arg(StyleHelper::toHexString(c.mutedForeground))
                    .arg(StyleHelper::toHexString(c.muted))
                    .arg(StyleHelper::toHexString(c.border)));
  update();
}

} // namespace ui
