#include "splitter.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QKeyEvent>
#include <QSplitterHandle>

namespace ui {
namespace {

class KeyboardSplitterHandle final : public QSplitterHandle {
public:
  using QSplitterHandle::QSplitterHandle;

protected:
  void keyPressEvent(QKeyEvent *event) override {
    const bool horizontal = orientation() == Qt::Horizontal;
    const int key = event->key();
    int direction = 0;
    if ((horizontal && key == Qt::Key_Left) ||
        (!horizontal && key == Qt::Key_Up)) {
      direction = -1;
    } else if ((horizontal && key == Qt::Key_Right) ||
               (!horizontal && key == Qt::Key_Down)) {
      direction = 1;
    } else {
      QSplitterHandle::keyPressEvent(event);
      return;
    }

    int position = horizontal ? x() : y();
    // The native handle helpers map RTL positions using the right-edge anchor.
    if (horizontal && splitter()->isRightToLeft()) {
      position += width();
    }
    const int step = Theme::instance().spacing().space2;
    moveSplitter(closestLegalPosition(position + direction * step));
    event->accept();
  }
};

} // namespace

Splitter::Splitter(QWidget *parent) : Splitter(Qt::Horizontal, parent) {}

Splitter::Splitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent) {
  setChildrenCollapsible(false);
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Splitter::applyThemeStyles);
  applyThemeStyles();
}

QSplitterHandle *Splitter::createHandle() {
  auto *handle = new KeyboardSplitterHandle(orientation(), this);
  handle->setFocusPolicy(Qt::StrongFocus);
  handle->setAttribute(Qt::WA_Hover, true);
  handle->setAccessibleName(tr("Resize panels"));
  return handle;
}

void Splitter::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &s = theme.spacing();
  const auto &r = theme.radius();

  setHandleWidth(s.space2);
  setStyleSheet(QString("ui--Splitter {"
                        "  background-color: transparent;"
                        "  border: none;"
                        "}"
                        "ui--Splitter::handle {"
                        "  background-color: %1;"
                        "  border: 1px solid %1;"
                        "  border-radius: %2px;"
                        "}"
                        "ui--Splitter::handle:hover {"
                        "  background-color: %3;"
                        "  border-color: %3;"
                        "}"
                        "ui--Splitter::handle:focus {"
                        "  background-color: %4;"
                        "  border-color: %4;"
                        "}"
                        "ui--Splitter::handle:pressed {"
                        "  background-color: %5;"
                        "  border-color: %5;"
                        "}"
                        "ui--Splitter::handle:disabled {"
                        "  background-color: %6;"
                        "  border-color: %1;"
                        "}")
                    .arg(StyleHelper::toHexString(c.border))
                    .arg(r.sm)
                    .arg(StyleHelper::toHexString(c.mutedForeground))
                    .arg(StyleHelper::toHexString(c.ring))
                    .arg(StyleHelper::toHexString(c.primary))
                    .arg(StyleHelper::toHexString(c.muted)));
}

} // namespace ui
