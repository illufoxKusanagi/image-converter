#include "separator.h"
#include "../theme/style_helper.h"

namespace ui {

Separator::Separator(Qt::Orientation orientation, QWidget *parent)
    : QFrame(parent), m_orientation(orientation) {
  setOrientation(orientation);
  connect(&Theme::instance(), &Theme::themeChanged, this, &Separator::applyThemeStyles);
  applyThemeStyles();
}

Qt::Orientation Separator::orientation() const {
  return m_orientation;
}

void Separator::setOrientation(Qt::Orientation orientation) {
  m_orientation = orientation;
  if (m_orientation == Qt::Horizontal) {
    setFrameShape(QFrame::HLine);
    setFixedHeight(1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  } else {
    setFrameShape(QFrame::VLine);
    setFixedWidth(1);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  }
  setFrameShadow(QFrame::Plain);
  applyThemeStyles();
}

void Separator::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  setStyleSheet(QString("background-color: %1; border: none;")
                    .arg(StyleHelper::toHexString(c.border)));
}

} // namespace ui

