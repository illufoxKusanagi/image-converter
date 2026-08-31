#include "switch.h"
#include "../theme/animation_helper.h"
#include "../theme/style_helper.h"
#include <QPainter>
#include <QPainterPath>

namespace ui {

Switch::Switch(QWidget *parent)
    : Switch(false, parent) {}

Switch::Switch(bool checked, QWidget *parent)
    : QAbstractButton(parent) {
  setCheckable(true);
  setChecked(checked);
  setCursor(Qt::PointingHandCursor);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_thumbOffset = checked ? 1.0 : 0.0;

  m_animation = new QVariantAnimation(this);
  m_animation->setDuration(AnimationHelper::DurationFast);
  m_animation->setEasingCurve(AnimationHelper::standardEasing());

  connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
    m_thumbOffset = value.toReal();
    update();
  });

  connect(&Theme::instance(), &Theme::themeChanged, this, [this]() {
    update();
  });
}

qreal Switch::thumbOffset() const {
  return m_thumbOffset;
}

void Switch::setThumbOffset(qreal offset) {
  m_thumbOffset = offset;
  update();
}

QSize Switch::sizeHint() const {
  return QSize(44, 24);
}

void Switch::checkStateSet() {
  QAbstractButton::checkStateSet();
  updateThumbAnimation(isChecked());
}

void Switch::nextCheckState() {
  QAbstractButton::nextCheckState();
  updateThumbAnimation(isChecked());
}

void Switch::updateThumbAnimation(bool checked) {
  m_animation->stop();
  m_animation->setStartValue(m_thumbOffset);
  m_animation->setEndValue(checked ? 1.0 : 0.0);
  m_animation->start();
}

void Switch::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  const auto &c = Theme::instance().colors();
  QRectF trackRect(0, 0, 44, 24);
  qreal radius = 12.0;

  QColor uncheckedColor = c.input;
  QColor checkedColor = c.primary;
  QColor activeTrackColor = isChecked() ? checkedColor : uncheckedColor;

  if (m_animation->state() == QAbstractAnimation::Running) {
    int r = uncheckedColor.red() + static_cast<int>((checkedColor.red() - uncheckedColor.red()) * m_thumbOffset);
    int g = uncheckedColor.green() + static_cast<int>((checkedColor.green() - uncheckedColor.green()) * m_thumbOffset);
    int b = uncheckedColor.blue() + static_cast<int>((checkedColor.blue() - uncheckedColor.blue()) * m_thumbOffset);
    activeTrackColor = QColor(r, g, b);
  }

  painter.setPen(Qt::NoPen);
  painter.setBrush(activeTrackColor);
  painter.drawRoundedRect(trackRect, radius, radius);

  if (hasFocus()) {
    QPen ringPen(c.ring, 2);
    painter.setPen(ringPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(trackRect.adjusted(-2, -2, 2, 2), radius + 2, radius + 2);
    painter.setPen(Qt::NoPen);
  }

  qreal thumbX = 2.0 + m_thumbOffset * (44.0 - 20.0 - 4.0);
  QRectF thumbRect(thumbX, 2.0, 20.0, 20.0);

  QColor thumbColor = isChecked() ? c.primaryForeground : c.background;
  painter.setBrush(thumbColor);
  painter.drawEllipse(thumbRect);
}

} // namespace ui
