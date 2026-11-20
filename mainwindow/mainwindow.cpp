#include "mainwindow.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QApplication>
#include <QDesktopServices>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>
#include <QVariantAnimation>
#include <algorithm>
#include <functional>
#include <utility>

namespace {
// Placeholder support URLs — replace with final Trakteer and Ko-fi links.
constexpr const char *kTrakteerUrl = "https://trakteer.id/illufox_kusanagi/tip";
// constexpr const char *kKofiUrl = "https://trakteer.id/indah_sinara/tipid";
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setMinimumSize(460, 620);
  setWindowTitle(tr("ImageConverter"));

  m_centralWidget = new QWidget(this);
  m_centralWidget->setObjectName(QStringLiteral("centralWidget"));
  QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);
  rootLayout->setContentsMargins(20, 18, 20, 20);
  rootLayout->setSpacing(18);

  // Header: Title, Support Links, Language Toggle & Theme Toggle
  QWidget *headerWidget = new QWidget(m_centralWidget);
  QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(0, 0, 0, 0);

  m_titleLabel = new QLabel(tr("ImageConverter"), headerWidget);
  m_titleLabel->setFont(ui::Theme::instance().typography().font(
      ui::Theme::instance().typography().sizeXl, QFont::Bold));

  m_trakteerBtn = new ui::Button("", ui::ButtonVariant::Ghost,
                                 ui::ButtonSize::Icon, headerWidget);
  m_trakteerBtn->setIconSize(
      QSize(kTrakteerCanvasLogicalSize, kTrakteerCanvasLogicalSize));
  connect(m_trakteerBtn, &QPushButton::clicked, this,
          &MainWindow::onTrakteerClicked);
  m_trakteerBtn->installEventFilter(this);

  {
    const QPixmap src(QStringLiteral(":/icons/icons/trakteer.png"));
    if (!src.isNull()) {
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
      clipPath.addRoundedRect(QRectF(0.0, 0.0, kTrakteerBadgeLogicalSize,
                                     kTrakteerBadgeLogicalSize),
                              kTrakteerBadgeCornerRadius,
                              kTrakteerBadgeCornerRadius);
      painter.setClipPath(clipPath);
      const QRectF srcRect((scaled.width() - physicalSize) / 2.0,
                           (scaled.height() - physicalSize) / 2.0, physicalSize,
                           physicalSize);
      painter.drawPixmap(QRectF(0.0, 0.0, kTrakteerBadgeLogicalSize,
                                kTrakteerBadgeLogicalSize),
                         scaled, srcRect);
      painter.end();
      m_trakteerBadgePixmap = badge;
    }
  }

  m_trakteerWiggleAnim = new QVariantAnimation(this);
  m_trakteerWiggleAnim->setDuration(kTrakteerWiggleDurationMs);
  m_trakteerWiggleAnim->setEasingCurve(QEasingCurve::InOutSine);
  m_trakteerWiggleAnim->setKeyValueAt(0.0, 0.0);
  m_trakteerWiggleAnim->setKeyValueAt(0.15, -12.0);
  m_trakteerWiggleAnim->setKeyValueAt(0.35, 12.0);
  m_trakteerWiggleAnim->setKeyValueAt(0.55, -8.0);
  m_trakteerWiggleAnim->setKeyValueAt(0.72, 8.0);
  m_trakteerWiggleAnim->setKeyValueAt(0.88, -4.0);
  m_trakteerWiggleAnim->setKeyValueAt(1.0, 0.0);
  connect(m_trakteerWiggleAnim, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &value) {
            updateTrakteerBadgeIcon(value.toReal());
          });
  connect(m_trakteerWiggleAnim, &QVariantAnimation::finished, this,
          [this]() { updateTrakteerBadgeIcon(0.0); });

  m_trakteerWiggleTimer = new QTimer(this);
  m_trakteerWiggleTimer->setInterval(kTrakteerWiggleIntervalMs);
  connect(m_trakteerWiggleTimer, &QTimer::timeout, this,
          &MainWindow::triggerTrakteerWiggle);
  m_trakteerWiggleTimer->start();

  // m_kofiBtn = new ui::Button("", ui::ButtonVariant::Ghost,
  // ui::ButtonSize::Icon,
  //                            headerWidget);
  // m_kofiBtn->setIconSize(QSize(16, 16));
  // connect(m_kofiBtn, &QPushButton::clicked, this,
  // &MainWindow::onKofiClicked);

  m_langToggleBtn = new ui::Button(ui::I18n::instance().displayBadge(),
                                   ui::ButtonVariant::Ghost,
                                   ui::ButtonSize::Small, headerWidget);
  connect(m_langToggleBtn, &QPushButton::clicked, this,
          &MainWindow::onLanguageToggleClicked);

  m_themeToggleBtn = new ui::Button(
      ui::Theme::instance().isDark() ? tr("Light") : tr("Dark"),
      ui::ButtonVariant::Ghost, ui::ButtonSize::Small, headerWidget);
  connect(m_themeToggleBtn, &QPushButton::clicked, this,
          &MainWindow::onThemeToggleClicked);

  headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
  headerLayout->addStretch(1);
  headerLayout->addWidget(m_trakteerBtn, 0, Qt::AlignVCenter);
  // headerLayout->addWidget(m_kofiBtn, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_langToggleBtn, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_themeToggleBtn, 0, Qt::AlignVCenter);
  rootLayout->addWidget(headerWidget);

  // Main Tabs: Convert Image, Compress PDF, UI Components
  m_tabs = new ui::Tabs(ui::TabVariant::Underline, m_centralWidget);

  m_mainPage = new MainPage(m_tabs);
  m_tabs->addTab(tr("Convert Image"), QString(":/icons/icons/lucide-image.svg"),
                 m_mainPage);

  m_pdfPage = new PdfPage(m_tabs);
  m_tabs->addTab(tr("PDF Tools"), QString(":/icons/icons/lucide-file-text.svg"),
                 m_pdfPage);

  // m_galleryPage = new GalleryPage(m_tabs);
  // m_tabs->addTab("UI Components",
  // QString(":/icons/icons/lucide-sliders.svg"),
  //                m_galleryPage);

  rootLayout->addWidget(m_tabs, 1);
  setCentralWidget(m_centralWidget);

  m_supportCallout = new SupportCalloutWidget(
      m_centralWidget, [this]() { onTrakteerClicked(); });
  m_supportCallout->hide();

  m_supportCalloutAnim = new QVariantAnimation(this);
  connect(m_supportCalloutAnim, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &value) {
            if (m_supportCallout) {
              static_cast<SupportCalloutWidget *>(m_supportCallout)
                  ->setOpacity(value.toReal());
            }
          });
  connect(m_supportCalloutAnim, &QVariantAnimation::finished, this, [this]() {
    if (m_supportCalloutHiding) {
      m_supportCalloutHiding = false;
      if (m_supportCallout) {
        m_supportCallout->hide();
      }
      scheduleNextSupportCallout();
    }
  });

  m_supportCalloutDelayTimer = new QTimer(this);
  m_supportCalloutDelayTimer->setSingleShot(true);
  connect(m_supportCalloutDelayTimer, &QTimer::timeout, this,
          &MainWindow::showSupportCallout);

  m_supportCalloutHideTimer = new QTimer(this);
  m_supportCalloutHideTimer->setSingleShot(true);
  connect(m_supportCalloutHideTimer, &QTimer::timeout, this,
          [this]() { hideSupportCallout(false); });

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainWindow::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &MainWindow::retranslateUi);
  retranslateUi();
  scheduleNextSupportCallout();
}

void MainWindow::onThemeToggleClicked() { ui::Theme::instance().toggleMode(); }

void MainWindow::onLanguageToggleClicked() {
  ui::I18n::instance().toggleLanguage();
}

void MainWindow::onTrakteerClicked() {
  m_supportCalloutSuppressed = true;
  if (m_supportCalloutDelayTimer) {
    m_supportCalloutDelayTimer->stop();
  }
  hideSupportCallout(true);
  QDesktopServices::openUrl(QUrl(QString::fromLatin1(kTrakteerUrl)));
}

// void MainWindow::onKofiClicked() {
//   QDesktopServices::openUrl(QUrl(QString::fromLatin1(kKofiUrl)));
// }

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_trakteerBtn && event->type() == QEvent::Enter) {
    triggerTrakteerWiggle();
    if (m_trakteerWiggleTimer) {
      m_trakteerWiggleTimer->start();
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  if (m_supportCallout && m_supportCallout->isVisible()) {
    positionSupportCallout();
  }
}

void MainWindow::triggerTrakteerWiggle() {
  if (!isVisible() || !m_trakteerWiggleAnim ||
      m_trakteerWiggleAnim->state() == QAbstractAnimation::Running) {
    return;
  }
  m_trakteerWiggleAnim->start();
}

void MainWindow::updateTrakteerBadgeIcon(qreal angleDegrees) {
  if (!m_trakteerBtn || m_trakteerBadgePixmap.isNull()) {
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
                     m_trakteerBadgePixmap);
  painter.end();

  m_trakteerBtn->setIcon(QIcon(canvas));
}

void MainWindow::scheduleNextSupportCallout() {
  if (m_supportCalloutSuppressed ||
      m_supportCalloutShownCount >= kCalloutMaxPerSession ||
      !m_supportCalloutDelayTimer) {
    return;
  }
  const bool fastTest = kUseFastCalloutTestMode ||
                        (qEnvironmentVariable("IMAGECONVERTER_TEST_CALLOUT") ==
                         QLatin1String("1"));
  const int minMs = fastTest ? kCalloutTestMinDelayMs : kCalloutMinDelayMs;
  const int maxMs = fastTest ? kCalloutTestMaxDelayMs : kCalloutMaxDelayMs;
  const int delayMs = QRandomGenerator::global()->bounded(minMs, maxMs + 1);
  m_supportCalloutDelayTimer->start(delayMs);
}

void MainWindow::showSupportCallout() {
  if (m_supportCalloutSuppressed ||
      m_supportCalloutShownCount >= kCalloutMaxPerSession ||
      !m_supportCallout || !m_supportCalloutAnim) {
    return;
  }
  if (!isVisible() || isMinimized() ||
      QApplication::activeModalWidget() != nullptr) {
    scheduleNextSupportCallout();
    return;
  }

  ++m_supportCalloutShownCount;

  auto *callout = static_cast<SupportCalloutWidget *>(m_supportCallout);
  callout->setText(tr("Support us!"));
  positionSupportCallout();
  m_supportCallout->show();
  m_supportCallout->raise();

  triggerTrakteerWiggle();
  if (m_trakteerWiggleTimer) {
    m_trakteerWiggleTimer->start();
  }

  m_supportCalloutHiding = false;
  m_supportCalloutAnim->stop();
  m_supportCalloutAnim->setDuration(kCalloutFadeInMs);
  m_supportCalloutAnim->setEasingCurve(QEasingCurve::OutCubic);
  m_supportCalloutAnim->setStartValue(0.0);
  m_supportCalloutAnim->setEndValue(1.0);
  callout->setOpacity(0.0);
  m_supportCalloutAnim->start();

  if (m_supportCalloutHideTimer) {
    m_supportCalloutHideTimer->start(kCalloutVisibleMs);
  }
}

void MainWindow::hideSupportCallout(bool immediate) {
  if (m_supportCalloutHideTimer) {
    m_supportCalloutHideTimer->stop();
  }
  if (!m_supportCallout || !m_supportCallout->isVisible()) {
    return;
  }
  auto *callout = static_cast<SupportCalloutWidget *>(m_supportCallout);
  if (immediate || !m_supportCalloutAnim) {
    if (m_supportCalloutAnim) {
      m_supportCalloutAnim->stop();
    }
    m_supportCalloutHiding = false;
    callout->setOpacity(0.0);
    m_supportCallout->hide();
    return;
  }

  m_supportCalloutAnim->stop();
  m_supportCalloutHiding = true;
  m_supportCalloutAnim->setDuration(kCalloutFadeOutMs);
  m_supportCalloutAnim->setEasingCurve(QEasingCurve::InCubic);
  m_supportCalloutAnim->setStartValue(callout->opacity());
  m_supportCalloutAnim->setEndValue(0.0);
  m_supportCalloutAnim->start();
}

void MainWindow::positionSupportCallout() {
  if (!m_supportCallout || !m_trakteerBtn || !m_centralWidget) {
    return;
  }
  auto *callout = static_cast<SupportCalloutWidget *>(m_supportCallout);
  const QPoint btnBottomCenter =
      m_trakteerBtn->mapTo(m_centralWidget, QPoint(m_trakteerBtn->width() / 2,
                                                   m_trakteerBtn->height()));
  const int bubbleW = callout->width();
  const int maxX =
      std::max(kCalloutWindowEdgeMargin,
               m_centralWidget->width() - bubbleW - kCalloutWindowEdgeMargin);
  const int desiredX = btnBottomCenter.x() - bubbleW / 2;
  const int bubbleX = std::clamp(desiredX, kCalloutWindowEdgeMargin, maxX);
  const int bubbleY = btnBottomCenter.y() + kCalloutTopGap;

  callout->move(bubbleX, bubbleY);
  callout->setArrowCenterX(static_cast<qreal>(btnBottomCenter.x() - bubbleX));
}

void MainWindow::retranslateUi() {
  setWindowTitle(tr("ImageConverter"));
  if (m_titleLabel) {
    m_titleLabel->setText(tr("ImageConverter"));
  }
  if (m_tabs) {
    m_tabs->setTabText(0, tr("Convert Image"));
    m_tabs->setTabText(1, tr("PDF Tools"));
  }
  if (m_trakteerBtn) {
    const QString label = tr("Support on Trakteer");
    m_trakteerBtn->setToolTip(label);
    m_trakteerBtn->setAccessibleName(label);
  }
  // if (m_kofiBtn) {
  //   const QString label = tr("Support on Ko-fi");
  //   m_kofiBtn->setToolTip(label);
  //   m_kofiBtn->setAccessibleName(label);
  // }
  if (m_langToggleBtn) {
    m_langToggleBtn->setText(ui::I18n::instance().displayBadge());
    m_langToggleBtn->setToolTip(
        tr("Switch Language (English / Bahasa Indonesia)"));
  }
  if (m_supportCallout) {
    auto *callout = static_cast<SupportCalloutWidget *>(m_supportCallout);
    callout->setText(tr("Support us!"));
    if (m_supportCallout->isVisible()) {
      positionSupportCallout();
    }
  }
  applyThemeStyles();
}

void MainWindow::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  ui::ThemeMode mode = ui::Theme::instance().mode();

  setStyleSheet(QString("QMainWindow { background-color: %1; }")
                    .arg(ui::StyleHelper::toHexString(c.background)));

  m_centralWidget->setStyleSheet(
      QString("#centralWidget { background-color: %1; }")
          .arg(ui::StyleHelper::toHexString(c.background)));

  m_titleLabel->setStyleSheet(
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground)));

  if (m_trakteerBtn) {
    m_trakteerBtn->setFixedSize(kHeaderIconButtonSize, kHeaderIconButtonSize);
    const qreal currentAngle =
        (m_trakteerWiggleAnim &&
         m_trakteerWiggleAnim->state() == QAbstractAnimation::Running)
            ? m_trakteerWiggleAnim->currentValue().toReal()
            : 0.0;
    updateTrakteerBadgeIcon(currentAngle);
  }
  // if (m_kofiBtn) {
  //   m_kofiBtn->setFixedSize(kHeaderIconButtonSize, kHeaderIconButtonSize);
  //   m_kofiBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-coffee.svg",
  //                                    c.foreground, QSize(16, 16)));
  // }
  if (m_supportCallout) {
    auto *callout = static_cast<SupportCalloutWidget *>(m_supportCallout);
    callout->setText(tr("Support us!"));
    if (m_supportCallout->isVisible()) {
      positionSupportCallout();
    }
    m_supportCallout->update();
  }

  QString modeText;
  QString iconPath;
  if (mode == ui::ThemeMode::Light) {
    modeText = tr("Light");
    iconPath = ":/icons/icons/lucide-sun.svg";
  } else if (mode == ui::ThemeMode::Dark) {
    modeText = tr("Dark");
    iconPath = ":/icons/icons/lucide-moon.svg";
  } else {
    modeText = tr("System");
    iconPath = ":/icons/icons/lucide-monitor.svg";
  }

  m_themeToggleBtn->setText(modeText);
  m_themeToggleBtn->setIcon(
      ui::Icon::get(iconPath, c.foreground, QSize(16, 16)));
}