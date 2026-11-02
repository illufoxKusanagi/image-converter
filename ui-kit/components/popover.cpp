#include "popover.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"

#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace ui {

Popover::Popover(QWidget *parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint),
      m_layout(new QVBoxLayout(this)) {
  setAttribute(Qt::WA_StyledBackground, true);
  setAttribute(Qt::WA_DeleteOnClose, false);
  setFrameShape(QFrame::NoFrame);
  setFocusPolicy(Qt::StrongFocus);
  setAccessibleName(tr("Popover"));
  // Allow the outer popup to fit the screen even when content needs scrolling.
  m_layout->setSizeConstraint(QLayout::SetNoConstraint);

  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Popover::applyThemeStyles);
  connect(qApp, &QGuiApplication::applicationStateChanged, this,
          [this](Qt::ApplicationState state) {
            if (state != Qt::ApplicationActive) {
              m_allowFocusRestore = false;
              hide();
            }
          });
  applyThemeStyles();
}

Popover::~Popover() {
  m_allowFocusRestore = false;
  detachAnchor();
}

QVBoxLayout *Popover::contentLayout() const { return m_layout; }

void Popover::openFor(QWidget *anchor) {
  if (!anchor || anchor == this || isAncestorOf(anchor) ||
      !anchor->isVisible() || !anchor->isEnabled()) {
    closePopover();
    return;
  }

  if (isVisible() && m_anchor == anchor) {
    m_layout->activate();
    positionForAnchor();
    raise();
    return;
  }

  detachAnchor();
  ++m_openGeneration;
  m_anchor = anchor;
  m_anchorWindow = anchor->window();
  QWidget *focused = QApplication::focusWidget();
  if (!isVisible() || (focused && focused != this && !isAncestorOf(focused))) {
    m_previousFocus = focused;
  }
  m_allowFocusRestore =
      QGuiApplication::applicationState() == Qt::ApplicationActive;
  anchor->installEventFilter(this);
  if (m_anchorWindow != anchor) {
    m_anchorWindow->installEventFilter(this);
  }
  m_anchorDestroyed = connect(anchor, &QObject::destroyed, this, [this]() {
    m_allowFocusRestore = false;
    detachAnchor();
    hide();
  });

  setLayoutDirection(anchor->layoutDirection());
  ensurePolished();
  m_layout->activate();
  positionForAnchor();
  QPointer<Popover> guard(this);
  show();
  if (guard && isVisible()) {
    raise();
  }
}

void Popover::closePopover() { hide(); }

void Popover::detachAnchor() {
  QObject::disconnect(m_anchorDestroyed);
  m_anchorDestroyed = QMetaObject::Connection();
  if (m_anchor) {
    m_anchor->removeEventFilter(this);
  }
  if (m_anchorWindow && m_anchorWindow != m_anchor) {
    m_anchorWindow->removeEventFilter(this);
  }
  m_anchor.clear();
  m_anchorWindow.clear();
}

bool Popover::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_anchor || watched == m_anchorWindow) {
    switch (event->type()) {
    case QEvent::Hide:
    case QEvent::Close:
      m_allowFocusRestore = false;
      hide();
      break;
    case QEvent::EnabledChange:
      if (m_anchor && !m_anchor->isEnabled()) {
        m_allowFocusRestore = false;
        hide();
      }
      break;
    case QEvent::Move:
    case QEvent::Resize:
    case QEvent::LayoutDirectionChange:
      if (isVisible()) {
        if (m_anchor) {
          setLayoutDirection(m_anchor->layoutDirection());
        }
        positionForAnchor();
      }
      break;
    default:
      break;
    }
  }
  return QFrame::eventFilter(watched, event);
}

void Popover::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    closePopover();
    event->accept();
    return;
  }
  QFrame::keyPressEvent(event);
}

void Popover::mousePressEvent(QMouseEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const QPoint localPos = event->position().toPoint();
  const QPoint globalPos = event->globalPosition().toPoint();
#else
  const QPoint localPos = event->pos();
  const QPoint globalPos = event->globalPos();
#endif
  bool suppressReplay = false;
  if (!rect().contains(localPos) && m_anchor) {
    QWidget *target = QApplication::widgetAt(globalPos);
    suppressReplay = m_anchor && target &&
                     (target == m_anchor || m_anchor->isAncestorOf(target));
  }
  // Set this before native popup dismissal, and only for the opening control.
  setAttribute(Qt::WA_NoMouseReplay, suppressReplay);
  QFrame::mousePressEvent(event);
}

void Popover::showEvent(QShowEvent *event) {
  QFrame::showEvent(event);
  if (!m_isOpen) {
    m_isOpen = true;
    QPointer<Popover> guard(this);
    emit opened();
    if (guard && isVisible() && m_isOpen) {
      QWidget *focused = QApplication::focusWidget();
      if (!focused || focused == this || !isAncestorOf(focused)) {
        focusInitialContent();
      }
    }
  }
}

void Popover::hideEvent(QHideEvent *event) {
  QFrame::hideEvent(event);
  if (!m_isOpen) {
    return;
  }
  m_isOpen = false;
  const QPointer<QWidget> target = m_previousFocus;
  const QPointer<QWidget> anchor = m_anchor;
  const quint64 generation = m_openGeneration;
  m_previousFocus.clear();
  detachAnchor();

  if (m_allowFocusRestore) {
    // Let native outside-click focus changes finish before considering
    // restoration.
    QTimer::singleShot(0, this, [this, target, anchor, generation]() {
      if (generation != m_openGeneration || isVisible() ||
          !m_allowFocusRestore ||
          QGuiApplication::applicationState() != Qt::ApplicationActive ||
          !target || !anchor || !anchor->isVisible() || !anchor->isEnabled() ||
          !target->isVisible() || !target->isEnabled() ||
          target->focusPolicy() == Qt::NoFocus ||
          target->window() != anchor->window() ||
          QApplication::activeWindow() != target->window() ||
          QApplication::activePopupWidget()) {
        return;
      }
      QWidget *focused = QApplication::focusWidget();
      if (focused && focused != target && focused != this &&
          !isAncestorOf(focused)) {
        return;
      }
      target->setFocus(Qt::PopupFocusReason);
    });
  }
  emit closed();
}

void Popover::changeEvent(QEvent *event) {
  QFrame::changeEvent(event);
  if (event->type() == QEvent::LayoutDirectionChange && isVisible()) {
    positionForAnchor();
  } else if (event->type() == QEvent::EnabledChange && !isEnabled()) {
    m_allowFocusRestore = false;
    hide();
  }
}

void Popover::focusInitialContent() {
  QWidget *candidate = nextInFocusChain();
  while (candidate && candidate != this) {
    if (isAncestorOf(candidate) && candidate->isVisible() &&
        candidate->isEnabled() && (candidate->focusPolicy() & Qt::TabFocus)) {
      candidate->setFocus(Qt::PopupFocusReason);
      return;
    }
    candidate = candidate->nextInFocusChain();
  }
  setFocus(Qt::PopupFocusReason);
}

void Popover::positionForAnchor() {
  if (!m_anchor) {
    return;
  }
  const QRect anchorRect(m_anchor->mapToGlobal(QPoint(0, 0)), m_anchor->size());
  QScreen *screen = QGuiApplication::screenAt(anchorRect.center());
  if (!screen) {
    screen = QGuiApplication::primaryScreen();
  }
  if (!screen) {
    return;
  }
  const QRect available = screen->availableGeometry();
  if (available.isEmpty()) {
    return;
  }

  const int gap = Theme::instance().spacing().space1;
  const QSize hint = sizeHint().expandedTo(minimumSizeHint());
  const int popupWidth =
      qBound(1, qMax(hint.width(), m_anchor->width()), available.width());
  const int desiredHeight = qMax(1, hint.height());
  const int belowY = anchorRect.bottom() + 1 + gap;
  const int aboveEnd = anchorRect.top() - gap;
  const int belowSpace = qMax(0, available.bottom() - belowY + 1);
  const int aboveSpace = qMax(0, aboveEnd - available.top());
  const bool below = desiredHeight <= belowSpace || belowSpace >= aboveSpace;
  const int room = qMin(available.height(), below ? belowSpace : aboveSpace);
  const int popupHeight =
      qMin(desiredHeight, room > 0 ? room : available.height());
  const int preferredX = layoutDirection() == Qt::RightToLeft
                             ? anchorRect.right() - popupWidth + 1
                             : anchorRect.left();
  const int preferredY =
      room > 0 ? (below ? belowY : aboveEnd - popupHeight) : available.top();
  const int x =
      qBound(available.left(), preferredX, available.right() - popupWidth + 1);
  const int y =
      qBound(available.top(), preferredY, available.bottom() - popupHeight + 1);
  setGeometry(x, y, popupWidth, popupHeight);
}

void Popover::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();
  setFont(t.font(t.sizeBase));
  m_layout->setContentsMargins(s.space3, s.space3, s.space3, s.space3);
  m_layout->setSpacing(s.space2);

  QPalette popupPalette = palette();
  popupPalette.setColor(QPalette::Window, c.popover);
  popupPalette.setColor(QPalette::WindowText, c.popoverForeground);
  popupPalette.setColor(QPalette::Base, c.popover);
  popupPalette.setColor(QPalette::Text, c.popoverForeground);
  popupPalette.setColor(QPalette::Button, c.secondary);
  popupPalette.setColor(QPalette::ButtonText, c.secondaryForeground);
  popupPalette.setColor(QPalette::Highlight, c.primary);
  popupPalette.setColor(QPalette::HighlightedText, c.primaryForeground);
  popupPalette.setColor(QPalette::Disabled, QPalette::WindowText,
                        c.mutedForeground);
  popupPalette.setColor(QPalette::Disabled, QPalette::Text, c.mutedForeground);
  popupPalette.setColor(QPalette::Disabled, QPalette::ButtonText,
                        c.mutedForeground);
  setPalette(popupPalette);
  setStyleSheet(QStringLiteral("ui--Popover {"
                               "  background-color: %1;"
                               "  color: %2;"
                               "  border: 1px solid %3;"
                               "  border-radius: %4px;"
                               "}")
                    .arg(StyleHelper::toHexString(c.popover))
                    .arg(StyleHelper::toHexString(c.popoverForeground))
                    .arg(StyleHelper::toHexString(c.border))
                    .arg(theme.radius().lg));
  if (isVisible()) {
    m_layout->activate();
    positionForAnchor();
  }
}

} // namespace ui
