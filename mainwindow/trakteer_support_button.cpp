#include "mainwindow/trakteer_support_button.h"
#include "ui-kit/theme/i18n.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QApplication>
#include <QDesktopServices>
#include <QEvent>
#include <QFontMetrics>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QRandomGenerator>
#include <QRectF>
#include <QSize>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVariantAnimation>
#include <algorithm>
#include <functional>
#include <utility>

namespace {
// Placeholder support URL — replace with final Trakteer link.
constexpr const char *kTrakteerUrl = "https://trakteer.id/illufox_kusanagi/tip";
constexpr int kHeaderIconButtonSize = 32;
constexpr qreal kTrakteerBadgeDpr = 2.0;
constexpr int kTrakteerBadgeLogicalSize = 18;
constexpr qreal kTrakteerBadgeCornerRadius = 4.5;
constexpr int kTrakteerCanvasLogicalSize = 24;
constexpr int kTrakteerWiggleDurationMs = 650;
constexpr int kTrakteerWiggleIntervalMs = 5000;

constexpr bool kUseFastCalloutTestMode = true;
constexpr int kCalloutMinDelayMs = 20000;
constexpr int kCalloutMaxDelayMs = 180000;
constexpr int kCalloutTestMinDelayMs = 1000;
constexpr int kCalloutTestMaxDelayMs = 5000;
constexpr int kCalloutMaxPerSession = 4;
constexpr int kCalloutVisibleMs = 3200;
constexpr int kCalloutFadeInMs = 180;
constexpr int kCalloutFadeOutMs = 220;

constexpr int kCalloutHorizontalPadding = 12;
constexpr int kCalloutVerticalPadding = 6;
constexpr qreal kCalloutArrowWidth = 10.0;
constexpr qreal kCalloutArrowHeight = 6.0;
constexpr qreal kCalloutCornerRadius = 8.0;
constexpr qreal kCalloutPaintMargin = 4.0;
constexpr int kCalloutTopGap = 4;
constexpr int kCalloutWindowEdgeMargin = 8;

class SupportCalloutWidget final : public QWidget {
public:
  explicit SupportCalloutWidget(QWidget *parent,
                                std::function<void()> onClicked)
      : QWidget(parent), m_onClicked(std::move(onClicked)) {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
  }

  void setOpacity(qreal opacity) {
    m_opacity = std::clamp(opacity, 0.0, 1.0);
    update();
  }

  qreal opacity() const { return m_opacity; }

  void setText(const QString &text) {
    m_text = text;
    setAccessibleName(text);

    const auto &t = ui::Theme::instance().typography();
    const QFont f = t.font(t.sizeXs, QFont::DemiBold);
    setFont(f);

    const QFontMetrics fm(f);
    const int textW = fm.horizontalAdvance(m_text);
    const int textH = fm.height();
    const int totalW = textW + 2 * kCalloutHorizontalPadding +
                       static_cast<int>(2.0 * kCalloutPaintMargin);
    const int totalH =
        textH + 2 * kCalloutVerticalPadding +
        static_cast<int>(kCalloutArrowHeight + 2.0 * kCalloutPaintMargin);
    setFixedSize(totalW, totalH);

    if (m_hasCustomArrowX) {
      m_arrowCenterX = clampArrowCenterX(m_arrowCenterX);
    } else {
      m_arrowCenterX = width() / 2.0;
    }
    update();
  }

  void setArrowCenterX(qreal x) {
    m_hasCustomArrowX = true;
    m_arrowCenterX = clampArrowCenterX(x);
    update();
  }

protected:
  void paintEvent(QPaintEvent * /*event*/) override {
    if (m_opacity <= 0.001) {
      return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setOpacity(m_opacity);

    const auto &c = ui::Theme::instance().colors();
    const auto &t = ui::Theme::instance().typography();
    const bool isDark = ui::Theme::instance().isDark();

    const qreal m = kCalloutPaintMargin;
    const qreal left = m;
    const qreal top = m + kCalloutArrowHeight;
    const qreal right = static_cast<qreal>(width()) - m;
    const qreal bottom = static_cast<qreal>(height()) - m;
    const qreal bodyW = right - left;
    const qreal bodyH = bottom - top;
    const qreal r = std::min({kCalloutCornerRadius, bodyW / 2.0, bodyH / 2.0});
    const qreal aw = kCalloutArrowWidth / 2.0;
    const qreal cx = clampArrowCenterX(m_arrowCenterX);

    QPainterPath path;
    path.moveTo(left + r, top);
    path.lineTo(cx - aw, top);
    path.lineTo(cx, m);
    path.lineTo(cx + aw, top);
    path.lineTo(right - r, top);
    path.arcTo(QRectF(right - 2.0 * r, top, 2.0 * r, 2.0 * r), 90.0, -90.0);
    path.lineTo(right, bottom - r);
    path.arcTo(QRectF(right - 2.0 * r, bottom - 2.0 * r, 2.0 * r, 2.0 * r), 0.0,
               -90.0);
    path.lineTo(left + r, bottom);
    path.arcTo(QRectF(left, bottom - 2.0 * r, 2.0 * r, 2.0 * r), 270.0, -90.0);
    path.lineTo(left, top + r);
    path.arcTo(QRectF(left, top, 2.0 * r, 2.0 * r), 180.0, -90.0);
    path.closeSubpath();

    const QColor shadowBase = isDark ? c.background : c.foreground;
    painter.save();
    painter.translate(0.0, 1.5);
    painter.setPen(
        QPen(ui::StyleHelper::withAlpha(shadowBase, isDark ? 48 : 16), 2.5,
             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(ui::StyleHelper::withAlpha(shadowBase, isDark ? 72 : 24));
    painter.drawPath(path);
    painter.restore();

    const QColor bubbleFill =
        ui::StyleHelper::tint(c.popover, c.info, isDark ? 0.16 : 0.08);
    const QColor borderColor =
        ui::StyleHelper::withAlpha(c.info, isDark ? 210 : 180);
    QPen borderPen(borderColor, 1.5, Qt::SolidLine, Qt::RoundCap,
                   Qt::RoundJoin);
    painter.setPen(borderPen);
    painter.setBrush(bubbleFill);
    painter.drawPath(path);

    const QRectF bodyRect(left, top, bodyW, bodyH);
    painter.setPen(c.popoverForeground);
    painter.setFont(t.font(t.sizeXs, QFont::DemiBold));
    painter.drawText(bodyRect, Qt::AlignCenter, m_text);
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() == Qt::LeftButton && m_onClicked) {
      m_onClicked();
      event->accept();
      return;
    }
    QWidget::mousePressEvent(event);
  }

private:
  qreal clampArrowCenterX(qreal x) const {
    const qreal m = kCalloutPaintMargin;
    const qreal left = m;
    const qreal right = static_cast<qreal>(width()) - m;
    const qreal bodyW = right - left;
    const qreal bodyH =
        static_cast<qreal>(height()) - 2.0 * m - kCalloutArrowHeight;
    const qreal r = std::min({kCalloutCornerRadius, bodyW / 2.0, bodyH / 2.0});
    const qreal aw = kCalloutArrowWidth / 2.0;
    const qreal minX = left + r + aw;
    const qreal maxX = right - r - aw;
    if (minX > maxX) {
      return (left + right) / 2.0;
    }
    return std::clamp(x, minX, maxX);
  }

  QString m_text;
  qreal m_opacity{0.0};
  qreal m_arrowCenterX{0.0};
  bool m_hasCustomArrowX{false};
  std::function<void()> m_onClicked;
};
} // namespace

TrakteerSupportButton::TrakteerSupportButton(QWidget *calloutHost,
                                             QWidget *parent)
    : ui::Button("", ui::ButtonVariant::Ghost, ui::ButtonSize::Icon, parent),
      m_calloutHost(calloutHost) {
  setIconSize(QSize(kTrakteerCanvasLogicalSize, kTrakteerCanvasLogicalSize));
  setFixedSize(kHeaderIconButtonSize, kHeaderIconButtonSize);

  initBadgePixmap();
  setupAnimations();

  if (m_calloutHost) {
    m_calloutHost->installEventFilter(this);

    QPointer<TrakteerSupportButton> self(this);
    auto *callout = new SupportCalloutWidget(m_calloutHost, [self]() {
      if (self) {
        self->onClicked();
      }
    });
    callout->hide();
    m_callout = callout;
    connect(this, &QObject::destroyed, m_callout, &QObject::deleteLater);
  }

  connect(this, &QPushButton::clicked, this,
          &TrakteerSupportButton::onClicked);
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &TrakteerSupportButton::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &TrakteerSupportButton::retranslateUi);

  retranslateUi();
  applyThemeStyles();
  scheduleNextCallout();
}

bool TrakteerSupportButton::event(QEvent *e) {
  if (e) {
    if (e->type() == QEvent::Enter) {
      triggerWiggle();
      if (m_wiggleTimer) {
        m_wiggleTimer->start();
      }
    } else if (e->type() == QEvent::Move || e->type() == QEvent::Resize) {
      if (m_callout && m_callout->isVisible()) {
        positionCallout();
      }
    }
  }
  return ui::Button::event(e);
}

bool TrakteerSupportButton::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_calloutHost && event && event->type() == QEvent::Resize) {
    if (m_callout && m_callout->isVisible()) {
      positionCallout();
    }
  }
  return ui::Button::eventFilter(watched, event);
}

void TrakteerSupportButton::onClicked() {
  m_calloutSuppressed = true;
  if (m_calloutDelayTimer) {
    m_calloutDelayTimer->stop();
  }
  hideCallout(true);
  QDesktopServices::openUrl(QUrl(QString::fromLatin1(kTrakteerUrl)));
}

void TrakteerSupportButton::initBadgePixmap() {
  const QPixmap src(QStringLiteral(":/icons/icons/trakteer.png"));
  if (src.isNull()) {
    return;
  }

  const int physicalSize =
      static_cast<int>(kTrakteerBadgeLogicalSize * kTrakteerBadgeDpr);
  const QPixmap scaled =
      src.scaled(physicalSize, physicalSize, Qt::KeepAspectRatioByExpanding,
                 Qt::SmoothTransformation);
  QPixmap badge(physicalSize, physicalSize);
  badge.fill(Qt::transparent);
  badge.setDevicePixelRatio(kTrakteerBadgeDpr);

  QPainter painter(&badge);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  QPainterPath clipPath;
  clipPath.addRoundedRect(
      QRectF(0.0, 0.0, kTrakteerBadgeLogicalSize, kTrakteerBadgeLogicalSize),
      kTrakteerBadgeCornerRadius, kTrakteerBadgeCornerRadius);
  painter.setClipPath(clipPath);
  const QRectF srcRect((scaled.width() - physicalSize) / 2.0,
                       (scaled.height() - physicalSize) / 2.0, physicalSize,
                       physicalSize);
  painter.drawPixmap(
      QRectF(0.0, 0.0, kTrakteerBadgeLogicalSize, kTrakteerBadgeLogicalSize),
      scaled, srcRect);
  painter.end();
  m_badgePixmap = badge;
}

void TrakteerSupportButton::setupAnimations() {
  m_wiggleAnim = new QVariantAnimation(this);
  m_wiggleAnim->setDuration(kTrakteerWiggleDurationMs);
  m_wiggleAnim->setEasingCurve(QEasingCurve::InOutSine);
  m_wiggleAnim->setKeyValueAt(0.0, 0.0);
  m_wiggleAnim->setKeyValueAt(0.15, -12.0);
  m_wiggleAnim->setKeyValueAt(0.35, 12.0);
  m_wiggleAnim->setKeyValueAt(0.55, -8.0);
  m_wiggleAnim->setKeyValueAt(0.72, 8.0);
  m_wiggleAnim->setKeyValueAt(0.88, -4.0);
  m_wiggleAnim->setKeyValueAt(1.0, 0.0);
  connect(m_wiggleAnim, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &value) {
            updateBadgeIcon(value.toReal());
          });
  connect(m_wiggleAnim, &QVariantAnimation::finished, this,
          [this]() { updateBadgeIcon(0.0); });

  m_wiggleTimer = new QTimer(this);
  m_wiggleTimer->setInterval(kTrakteerWiggleIntervalMs);
  connect(m_wiggleTimer, &QTimer::timeout, this,
          &TrakteerSupportButton::triggerWiggle);
  m_wiggleTimer->start();

  m_calloutAnim = new QVariantAnimation(this);
  connect(m_calloutAnim, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &value) {
            if (m_callout) {
              static_cast<SupportCalloutWidget *>(m_callout.data())
                  ->setOpacity(value.toReal());
            }
          });
  connect(m_calloutAnim, &QVariantAnimation::finished, this, [this]() {
    if (m_calloutHiding) {
      m_calloutHiding = false;
      if (m_callout) {
        m_callout->hide();
      }
      scheduleNextCallout();
    }
  });

  m_calloutDelayTimer = new QTimer(this);
  m_calloutDelayTimer->setSingleShot(true);
  connect(m_calloutDelayTimer, &QTimer::timeout, this,
          &TrakteerSupportButton::showCallout);

  m_calloutHideTimer = new QTimer(this);
  m_calloutHideTimer->setSingleShot(true);
  connect(m_calloutHideTimer, &QTimer::timeout, this,
          [this]() { hideCallout(false); });
}

void TrakteerSupportButton::triggerWiggle() {
  QWidget *topWin = window();
  if (!isVisible() || !topWin || !topWin->isVisible() ||
      topWin->isMinimized() || !m_wiggleAnim ||
      m_wiggleAnim->state() == QAbstractAnimation::Running) {
    return;
  }
  m_wiggleAnim->start();
}

void TrakteerSupportButton::updateBadgeIcon(qreal angleDegrees) {
  if (m_badgePixmap.isNull()) {
    return;
  }
  const int physicalCanvas =
      static_cast<int>(kTrakteerCanvasLogicalSize * kTrakteerBadgeDpr);
  QPixmap canvas(physicalCanvas, physicalCanvas);
  canvas.fill(Qt::transparent);
  canvas.setDevicePixelRatio(kTrakteerBadgeDpr);

  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.translate(kTrakteerCanvasLogicalSize / 2.0,
                    kTrakteerCanvasLogicalSize / 2.0);
  painter.rotate(angleDegrees);
  painter.drawPixmap(QPointF(-kTrakteerBadgeLogicalSize / 2.0,
                             -kTrakteerBadgeLogicalSize / 2.0),
                     m_badgePixmap);
  painter.end();

  setIcon(QIcon(canvas));
}

void TrakteerSupportButton::scheduleNextCallout() {
  if (m_calloutSuppressed || m_calloutShownCount >= kCalloutMaxPerSession ||
      !m_calloutDelayTimer || !m_calloutHost) {
    return;
  }
  const bool fastTest =
      kUseFastCalloutTestMode ||
      (qEnvironmentVariable("IMAGECONVERTER_TEST_CALLOUT") ==
       QLatin1String("1"));
  const int minMs = fastTest ? kCalloutTestMinDelayMs : kCalloutMinDelayMs;
  const int maxMs = fastTest ? kCalloutTestMaxDelayMs : kCalloutMaxDelayMs;
  const int delayMs = QRandomGenerator::global()->bounded(minMs, maxMs + 1);
  m_calloutDelayTimer->start(delayMs);
}

void TrakteerSupportButton::showCallout() {
  if (m_calloutSuppressed || m_calloutShownCount >= kCalloutMaxPerSession ||
      !m_callout || !m_calloutAnim || !m_calloutHost) {
    return;
  }
  QWidget *topWin = window();
  if (!isVisible() || !topWin || !topWin->isVisible() ||
      topWin->isMinimized() || QApplication::activeModalWidget() != nullptr) {
    scheduleNextCallout();
    return;
  }

  ++m_calloutShownCount;

  auto *callout = static_cast<SupportCalloutWidget *>(m_callout.data());
  callout->setText(tr("Support us!"));
  positionCallout();
  m_callout->show();
  m_callout->raise();

  triggerWiggle();
  if (m_wiggleTimer) {
    m_wiggleTimer->start();
  }

  m_calloutHiding = false;
  m_calloutAnim->stop();
  m_calloutAnim->setDuration(kCalloutFadeInMs);
  m_calloutAnim->setEasingCurve(QEasingCurve::OutCubic);
  m_calloutAnim->setStartValue(0.0);
  m_calloutAnim->setEndValue(1.0);
  callout->setOpacity(0.0);
  m_calloutAnim->start();

  if (m_calloutHideTimer) {
    m_calloutHideTimer->start(kCalloutVisibleMs);
  }
}

void TrakteerSupportButton::hideCallout(bool immediate) {
  if (m_calloutHideTimer) {
    m_calloutHideTimer->stop();
  }
  if (!m_callout || !m_callout->isVisible()) {
    return;
  }
  auto *callout = static_cast<SupportCalloutWidget *>(m_callout.data());
  if (immediate || !m_calloutAnim) {
    if (m_calloutAnim) {
      m_calloutAnim->stop();
    }
    m_calloutHiding = false;
    callout->setOpacity(0.0);
    m_callout->hide();
    return;
  }

  m_calloutAnim->stop();
  m_calloutHiding = true;
  m_calloutAnim->setDuration(kCalloutFadeOutMs);
  m_calloutAnim->setEasingCurve(QEasingCurve::InCubic);
  m_calloutAnim->setStartValue(callout->opacity());
  m_calloutAnim->setEndValue(0.0);
  m_calloutAnim->start();
}

void TrakteerSupportButton::positionCallout() {
  if (!m_callout || !m_calloutHost) {
    return;
  }
  auto *callout = static_cast<SupportCalloutWidget *>(m_callout.data());
  const QPoint centerLocal(width() / 2, height());
  const QPoint btnBottomCenter =
      m_calloutHost->isAncestorOf(this)
          ? mapTo(m_calloutHost.data(), centerLocal)
          : m_calloutHost->mapFromGlobal(mapToGlobal(centerLocal));
  const int bubbleW = callout->width();
  const int maxX =
      std::max(kCalloutWindowEdgeMargin,
               m_calloutHost->width() - bubbleW - kCalloutWindowEdgeMargin);
  const int desiredX = btnBottomCenter.x() - bubbleW / 2;
  const int bubbleX = std::clamp(desiredX, kCalloutWindowEdgeMargin, maxX);
  const int bubbleY = btnBottomCenter.y() + kCalloutTopGap;

  callout->move(bubbleX, bubbleY);
  callout->setArrowCenterX(static_cast<qreal>(btnBottomCenter.x() - bubbleX));
}

void TrakteerSupportButton::retranslateUi() {
  const QString label = tr("Support on Trakteer");
  setToolTip(label);
  setAccessibleName(label);

  if (m_callout) {
    auto *callout = static_cast<SupportCalloutWidget *>(m_callout.data());
    callout->setText(tr("Support us!"));
    if (m_callout->isVisible()) {
      positionCallout();
    }
  }
}

void TrakteerSupportButton::applyThemeStyles() {
  setFixedSize(kHeaderIconButtonSize, kHeaderIconButtonSize);
  const qreal currentAngle =
      (m_wiggleAnim && m_wiggleAnim->state() == QAbstractAnimation::Running)
          ? m_wiggleAnim->currentValue().toReal()
          : 0.0;
  updateBadgeIcon(currentAngle);

  if (m_callout) {
    auto *callout = static_cast<SupportCalloutWidget *>(m_callout.data());
    callout->setText(tr("Support us!"));
    if (m_callout->isVisible()) {
      positionCallout();
    }
    m_callout->update();
  }
}
