#include "avatar.h"
#include "../theme/style_helper.h"
#include <QPainter>
#include <QPainterPath>

namespace ui {

Avatar::Avatar(QWidget *parent)
    : Avatar("?", AvatarSize::Default, parent) {}

Avatar::Avatar(const QString &initials, AvatarSize size, QWidget *parent)
    : QWidget(parent), m_initials(initials), m_size(size) {
  setSize(size);
  connect(&Theme::instance(), &Theme::themeChanged, this, &Avatar::applyThemeStyles);
}

Avatar::Avatar(const QPixmap &image, AvatarSize size, QWidget *parent)
    : QWidget(parent), m_image(image), m_size(size) {
  setSize(size);
  connect(&Theme::instance(), &Theme::themeChanged, this, &Avatar::applyThemeStyles);
}

void Avatar::setImage(const QPixmap &pixmap) {
  m_image = pixmap;
  update();
}

void Avatar::setInitials(const QString &initials) {
  m_image = QPixmap{};
  m_initials = initials;
  update();
}

AvatarSize Avatar::size() const {
  return m_size;
}

void Avatar::setSize(AvatarSize size) {
  m_size = size;
  int d = diameter();
  setFixedSize(d, d);
  update();
}

AvatarStatus Avatar::status() const {
  return m_status;
}

void Avatar::setStatus(AvatarStatus status) {
  m_status = status;
  update();
}

int Avatar::diameter() const {
  switch (m_size) {
  case AvatarSize::Small:
    return 28;
  case AvatarSize::Large:
    return 56;
  case AvatarSize::Default:
  default:
    return 40;
  }
}

QSize Avatar::sizeHint() const {
  int d = diameter();
  return QSize(d, d);
}

void Avatar::applyThemeStyles() {
  update();
}

void Avatar::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  const auto &c = Theme::instance().colors();
  const auto &t = Theme::instance().typography();
  int d = diameter();
  QRectF circleRect(0, 0, d, d);

  QPainterPath circlePath;
  circlePath.addEllipse(circleRect);

  if (!m_image.isNull()) {
    painter.save();
    painter.setClipPath(circlePath);
    QPixmap scaled = m_image.scaled(d, d, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (d - scaled.width()) / 2;
    int y = (d - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    painter.restore();
  } else {
    // Background
    painter.fillPath(circlePath, c.muted);

    // Initials Text
    painter.setPen(c.mutedForeground);
    int fontSize = (m_size == AvatarSize::Small) ? t.sizeXs : (m_size == AvatarSize::Large) ? t.sizeLg : t.sizeSm;
    painter.setFont(t.font(fontSize, QFont::DemiBold));
    painter.drawText(circleRect, Qt::AlignCenter, m_initials.toUpper());
  }

  // Border outline
  painter.setPen(QPen(c.border, 1));
  painter.drawEllipse(circleRect.adjusted(0.5, 0.5, -0.5, -0.5));

  // Status Dot
  if (m_status != AvatarStatus::None) {
    int dotSize = (m_size == AvatarSize::Small) ? 8 : (m_size == AvatarSize::Large) ? 14 : 10;
    qreal ringOffset = 2.0;
    qreal dotX = d - dotSize - ringOffset;
    qreal dotY = d - dotSize - ringOffset;
    QRectF dotRect(dotX, dotY, dotSize, dotSize);

    QColor statusColor;
    switch (m_status) {
    case AvatarStatus::Online:
      statusColor = QColor("#22c55e");
      break;
    case AvatarStatus::Busy:
      statusColor = QColor("#ef4444");
      break;
    case AvatarStatus::Away:
      statusColor = QColor("#f59e0b");
      break;
    case AvatarStatus::Offline:
    default:
      statusColor = QColor("#6b7280");
      break;
    }

    // Outer background ring
    painter.setPen(Qt::NoPen);
    painter.setBrush(c.background);
    painter.drawEllipse(dotRect.adjusted(-1.5, -1.5, 1.5, 1.5));

    // Inner status fill
    painter.setBrush(statusColor);
    painter.drawEllipse(dotRect);
  }
}

} // namespace ui
