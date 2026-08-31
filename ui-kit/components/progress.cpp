#include "progress.h"
#include "../theme/animation_helper.h"
#include "../theme/style_helper.h"
#include <QPainter>
#include <QPainterPath>

namespace ui {

Progress::Progress(QWidget *parent)
    : Progress(0, parent) {}

Progress::Progress(int value, QWidget *parent)
    : QWidget(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  m_valueAnim = new QVariantAnimation(this);
  m_valueAnim->setDuration(AnimationHelper::DurationNormal);
  m_valueAnim->setEasingCurve(AnimationHelper::standardEasing());
  connect(m_valueAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
    m_displayValue = val.toInt();
    update();
  });

  m_indeterminateAnim = new QVariantAnimation(this);
  m_indeterminateAnim->setDuration(1200);
  m_indeterminateAnim->setStartValue(0.0);
  m_indeterminateAnim->setEndValue(1.0);
  m_indeterminateAnim->setLoopCount(-1);
  connect(m_indeterminateAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
    m_indeterminatePos = val.toReal();
    update();
  });

  setValue(value, false);

  connect(&Theme::instance(), &Theme::themeChanged, this, &Progress::applyThemeStyles);
  applyThemeStyles();
}

int Progress::value() const {
  return m_value;
}

void Progress::setValue(int value, bool animated) {
  int clamped = qBound(m_min, value, m_max);
  if (m_value == clamped) {
    return;
  }
  m_value = clamped;

  if (animated && isVisible()) {
    m_valueAnim->stop();
    m_valueAnim->setStartValue(m_displayValue);
    m_valueAnim->setEndValue(m_value);
    m_valueAnim->start();
  } else {
    m_valueAnim->stop();
    m_displayValue = m_value;
    update();
  }
}

int Progress::minimum() const {
  return m_min;
}

void Progress::setMinimum(int min) {
  setRange(min, m_max);
}

int Progress::maximum() const {
  return m_max;
}

void Progress::setMaximum(int max) {
  setRange(m_min, max);
}

void Progress::setRange(int min, int max) {
  m_min = min;
  m_max = qMax(min, max);
  setValue(m_value, false);
  update();
}

bool Progress::isIndeterminate() const {
  return m_isIndeterminate;
}

void Progress::setIndeterminate(bool indeterminate) {
  if (m_isIndeterminate == indeterminate) {
    return;
  }
  m_isIndeterminate = indeterminate;
  if (m_isIndeterminate) {
    m_indeterminateAnim->start();
  } else {
    m_indeterminateAnim->stop();
  }
  update();
}

ProgressSize Progress::size() const {
  return m_size;
}

void Progress::setSize(ProgressSize size) {
  m_size = size;
  updateGeometry();
  update();
}

int Progress::trackHeight() const {
  switch (m_size) {
  case ProgressSize::Small:
    return 4;
  case ProgressSize::Large:
    return 12;
  case ProgressSize::Default:
  default:
    return 8;
  }
}

QSize Progress::sizeHint() const {
  return QSize(120, trackHeight() + 8);
}

void Progress::applyThemeStyles() {
  update();
}

void Progress::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  const auto &c = Theme::instance().colors();
  int h = trackHeight();
  qreal y = (height() - h) / 2.0;
  qreal r = h / 2.0;

  // Track background
  QRectF trackRect(0, y, width(), h);
  QPainterPath trackPath;
  trackPath.addRoundedRect(trackRect, r, r);
  painter.fillPath(trackPath, c.secondary);

  if (m_isIndeterminate) {
    // Sliding pulse indicator
    qreal barWidth = width() * 0.35;
    qreal startX = -barWidth + (width() + barWidth) * m_indeterminatePos;
    QRectF fillRect(startX, y, barWidth, h);

    painter.save();
    painter.setClipPath(trackPath);
    QPainterPath fillPath;
    fillPath.addRoundedRect(fillRect, r, r);
    painter.fillPath(fillPath, c.primary);
    painter.restore();
  } else {
    // Determinate fill
    qreal range = m_max - m_min;
    qreal progress = (range > 0) ? static_cast<qreal>(m_displayValue - m_min) / range : 0.0;
    qreal fillWidth = width() * qBound(0.0, progress, 1.0);

    if (fillWidth > 0) {
      QRectF fillRect(0, y, fillWidth, h);
      QPainterPath fillPath;
      fillPath.addRoundedRect(fillRect, r, r);

      painter.save();
      painter.setClipPath(trackPath);
      painter.fillPath(fillPath, c.primary);
      painter.restore();
    }
  }
}

} // namespace ui
