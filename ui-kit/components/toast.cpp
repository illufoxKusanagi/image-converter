#include "toast.h"
#include "../theme/animation_helper.h"
#include "../theme/style_helper.h"
#include <QGraphicsDropShadowEffect>

namespace ui {

Toast::Toast(QWidget *parent)
    : Toast("", "", ToastVariant::Default, parent) {}

Toast::Toast(const QString &title, const QString &description, ToastVariant variant, QWidget *parent)
    : QFrame(parent), m_variant(variant) {
  setFixedWidth(340);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

  m_rootLayout = new QHBoxLayout(this);
  m_rootLayout->setContentsMargins(14, 12, 12, 12);
  m_rootLayout->setSpacing(10);

  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedSize(20, 20);
  m_iconLabel->setAlignment(Qt::AlignCenter);

  m_textLayout = new QVBoxLayout();
  m_textLayout->setContentsMargins(0, 0, 0, 0);
  m_textLayout->setSpacing(2);

  m_titleLabel = new QLabel(title, this);
  m_descLabel = new QLabel(description, this);
  m_descLabel->setWordWrap(true);
  m_descLabel->setVisible(!description.isEmpty());

  m_textLayout->addWidget(m_titleLabel);
  m_textLayout->addWidget(m_descLabel);

  m_closeButton = new QPushButton("✕", this);
  m_closeButton->setFixedSize(18, 18);
  m_closeButton->setCursor(Qt::PointingHandCursor);
  m_closeButton->setToolTip("Dismiss");
  connect(m_closeButton, &QPushButton::clicked, this, &Toast::dismiss);

  m_rootLayout->addWidget(m_iconLabel, 0, Qt::AlignTop);
  m_rootLayout->addLayout(m_textLayout, 1);
  m_rootLayout->addWidget(m_closeButton, 0, Qt::AlignTop);

  m_opacityEffect = new QGraphicsOpacityEffect(this);
  m_opacityEffect->setOpacity(0.0);
  setGraphicsEffect(m_opacityEffect);

  m_dismissTimer = new QTimer(this);
  m_dismissTimer->setSingleShot(true);
  connect(m_dismissTimer, &QTimer::timeout, this, &Toast::dismiss);

  m_anim = new QVariantAnimation(this);

  connect(&Theme::instance(), &Theme::themeChanged, this, &Toast::applyThemeStyles);
  applyThemeStyles();
}

void Toast::setTitle(const QString &title) {
  m_titleLabel->setText(title);
}

void Toast::setDescription(const QString &description) {
  m_descLabel->setText(description);
  m_descLabel->setVisible(!description.isEmpty());
}

void Toast::setVariant(ToastVariant variant) {
  if (m_variant != variant) {
    m_variant = variant;
    applyThemeStyles();
  }
}

void Toast::setDuration(int durationMs) {
  m_durationMs = durationMs;
}

void Toast::setupPosition() {
  if (parentWidget()) {
    int x = parentWidget()->width() - width() - 24;
    int y = parentWidget()->height() - height() - 24;
    move(qMax(0, x), qMax(0, y));
  }
}

void Toast::showToast() {
  adjustSize();
  setupPosition();
  QWidget::show();
  raise();

  m_anim->stop();
  m_anim->setDuration(AnimationHelper::DurationNormal);
  m_anim->setStartValue(0.0);
  m_anim->setEndValue(1.0);
  m_anim->setEasingCurve(AnimationHelper::standardEasing());

  connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
    m_opacityEffect->setOpacity(val.toReal());
  });

  m_anim->start();

  if (m_durationMs > 0) {
    m_dismissTimer->start(m_durationMs);
  }
}

void Toast::dismiss() {
  m_dismissTimer->stop();
  m_anim->stop();
  m_anim->setDuration(AnimationHelper::DurationFast);
  m_anim->setStartValue(m_opacityEffect->opacity());
  m_anim->setEndValue(0.0);
  m_anim->setEasingCurve(AnimationHelper::standardEasing());

  connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
    m_opacityEffect->setOpacity(val.toReal());
  });
  connect(m_anim, &QVariantAnimation::finished, this, &Toast::deleteLater);

  m_anim->start();
}

Toast *Toast::show(QWidget *parent, const QString &title, const QString &description, ToastVariant variant, int durationMs) {
  Toast *toast = new Toast(title, description, variant, parent);
  toast->setDuration(durationMs);
  toast->showToast();
  return toast;
}

void Toast::enterEvent(QEnterEvent *event) {
  Q_UNUSED(event);
  m_dismissTimer->stop();
}

void Toast::leaveEvent(QEvent *event) {
  Q_UNUSED(event);
  if (m_durationMs > 0) {
    m_dismissTimer->start(m_durationMs);
  }
}

void Toast::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  m_titleLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
  m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(StyleHelper::toHexString(c.foreground)));

  m_descLabel->setFont(t.font(t.sizeXs, QFont::Normal));
  m_descLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(StyleHelper::toHexString(c.mutedForeground)));

  m_closeButton->setStyleSheet(QString(
      "QPushButton {"
      "  background: transparent;"
      "  border: none;"
      "  color: %1;"
      "  font-size: 11px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  color: %2;"
      "}")
      .arg(StyleHelper::toHexString(c.mutedForeground))
      .arg(StyleHelper::toHexString(c.foreground)));

  QString iconSymbol;
  QColor iconColor = c.foreground;
  QColor borderCol = c.border;

  switch (m_variant) {
  case ToastVariant::Success:
    iconSymbol = "✓";
    iconColor = QColor("#22c55e");
    break;
  case ToastVariant::Warning:
    iconSymbol = "⚠";
    iconColor = QColor("#f59e0b");
    break;
  case ToastVariant::Destructive:
    iconSymbol = "✕";
    iconColor = c.destructive;
    borderCol = c.destructive;
    break;
  case ToastVariant::Default:
  default:
    iconSymbol = "ℹ";
    iconColor = c.primary;
    break;
  }

  m_iconLabel->setText(iconSymbol);
  m_iconLabel->setFont(t.font(t.sizeBase, QFont::Bold));
  m_iconLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(StyleHelper::toHexString(iconColor)));

  setStyleSheet(QString(
      "ui--Toast, QFrame {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: %3px;"
      "}")
      .arg(StyleHelper::toHexString(c.card))
      .arg(StyleHelper::toHexString(borderCol))
      .arg(r.lg));
}

} // namespace ui
