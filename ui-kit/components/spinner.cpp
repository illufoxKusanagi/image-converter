#include "spinner.h"
#include "../theme/style_helper.h"
#include <QPainter>
#include <QPainterPath>

namespace ui {

Spinner::Spinner(QWidget *parent)
    : Spinner(24, parent) {}

Spinner::Spinner(int size, QWidget *parent)
    : QWidget(parent), m_size(size) {
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setFixedSize(m_size, m_size);

  m_animation = new QVariantAnimation(this);
  m_animation->setStartValue(0);
  m_animation->setEndValue(360);
  m_animation->setDuration(800);
  m_animation->setLoopCount(-1);

  connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
    setAngle(value.toInt());
  });

  connect(&Theme::instance(), &Theme::themeChanged, this, [this]() {
    update();
  });

  start();
}

int Spinner::size() const {
  return m_size;
}

void Spinner::setSpinnerSize(int size) {
  if (m_size != size) {
    m_size = size;
    setFixedSize(m_size, m_size);
    update();
  }
}

QColor Spinner::color() const {
  return m_customColor.isValid() ? m_customColor : Theme::instance().colors().primary;
}

void Spinner::setColor(const QColor &color) {
  m_customColor = color;
  update();
}

int Spinner::angle() const {
  return m_angle;
}

void Spinner::setAngle(int angle) {
  m_angle = angle;
  update();
}

void Spinner::start() {
  if (m_animation->state() != QAbstractAnimation::Running) {
    m_animation->start();
  }
}

void Spinner::stop() {
  if (m_animation->state() == QAbstractAnimation::Running) {
    m_animation->stop();
  }
}

bool Spinner::isSpinning() const {
  return m_animation->state() == QAbstractAnimation::Running;
}

QSize Spinner::sizeHint() const {
  return QSize(m_size, m_size);
}

void Spinner::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  qreal penWidth = qMax(2.0, m_size / 8.0);
  QRectF drawRect = rect().adjusted(penWidth / 2.0, penWidth / 2.0, -penWidth / 2.0, -penWidth / 2.0);

  QColor activeColor = color();

  QPen trackPen(StyleHelper::withAlphaF(activeColor, 0.15), penWidth);
  trackPen.setCapStyle(Qt::RoundCap);
  painter.setPen(trackPen);
  painter.drawEllipse(drawRect);

  QPen arcPen(activeColor, penWidth);
  arcPen.setCapStyle(Qt::RoundCap);
  painter.setPen(arcPen);

  int startAngle = -m_angle * 16;
  int spanAngle = -100 * 16;
  painter.drawArc(drawRect, startAngle, spanAngle);
}

} // namespace ui
