#include "radio_group.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QBoxLayout>
#include <QButtonGroup>
#include <QVariant>
#include <limits>

namespace ui {

RadioGroup::RadioGroup(QWidget *parent) : RadioGroup(QStringList(), parent) {}

RadioGroup::RadioGroup(const QStringList &options, QWidget *parent)
    : QWidget(parent), m_group(new QButtonGroup(this)),
      m_layout(new QBoxLayout(QBoxLayout::TopToBottom, this)) {
  m_group->setExclusive(true);
  m_layout->setContentsMargins(0, 0, 0, 0);
  setFocusPolicy(Qt::StrongFocus);

  connect(&Theme::instance(), &Theme::themeChanged, this,
          &RadioGroup::applyThemeStyles);
  for (const QString &text : options) {
    addOption(text);
  }
  applyThemeStyles();
}

void RadioGroup::addOption(const QString &text, int id) {
  if (id < 0) {
    id = 0;
    while (m_group->button(id)) {
      if (id == std::numeric_limits<int>::max()) {
        return;
      }
      ++id;
    }
  } else if (m_group->button(id)) {
    return;
  }

  auto *option = new QRadioButton(text, this);
  option->setAutoExclusive(false);
  option->setFocusPolicy(Qt::StrongFocus);
  option->setProperty("_uiRadioGroupOption", true);
  m_group->addButton(option, id);
  if (m_layout->count() > 0) {
    QWidget::setTabOrder(m_layout->itemAt(m_layout->count() - 1)->widget(),
                         option);
  } else {
    setFocusProxy(option);
  }
  m_layout->addWidget(option);

  // QRadioButton supplies its text as the native accessible label and handles
  // arrows.
  connect(option, &QRadioButton::toggled, this, [this, option](bool checked) {
    if (checked) {
      setFocusProxy(option);
      emit selectionChanged(m_group->id(option));
    }
  });
  applyThemeStyles();
}

int RadioGroup::checkedId() const { return m_group->checkedId(); }

void RadioGroup::setCheckedId(int id) {
  if (id == -1) {
    if (auto *checked = m_group->checkedButton()) {
      m_group->setExclusive(false);
      checked->setChecked(false);
      m_group->setExclusive(true);
      emit selectionChanged(-1);
    }
    return;
  }
  if (auto *option = button(id)) {
    option->setChecked(true);
  }
}

QRadioButton *RadioGroup::button(int id) const {
  return qobject_cast<QRadioButton *>(m_group->button(id));
}

Qt::Orientation RadioGroup::orientation() const { return m_orientation; }

void RadioGroup::setOrientation(Qt::Orientation orientation) {
  if (m_orientation == orientation) {
    return;
  }
  m_orientation = orientation;
  m_layout->setDirection(orientation == Qt::Horizontal
                             ? QBoxLayout::LeftToRight
                             : QBoxLayout::TopToBottom);
  applyThemeStyles();
}

void RadioGroup::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &r = theme.radius();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();
  m_layout->setSpacing(m_orientation == Qt::Horizontal ? s.space4 : s.space2);

  const QString selector =
      QStringLiteral("QRadioButton[_uiRadioGroupOption=\"true\"]");
  const QString styles =
      QString("%1 { color: %2; spacing: %3px; }"
              "%1::indicator {"
              "  width: %4px;"
              "  height: %4px;"
              "  border: 1px solid %5;"
              "  border-radius: %6px;"
              "  background-color: %7;"
              "  image: none;"
              "}"
              "%1::indicator:checked {"
              "  border-color: %8;"
              "  background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5,"
              "    fx: 0.5, fy: 0.5, stop: 0 %8, stop: 0.45 %8,"
              "    stop: 0.5 %7, stop: 1 %7);"
              "}"
              "%1::indicator:hover { border-color: %8; }"
              "%1::indicator:focus { border-color: %9; }"
              "%1:disabled { color: %10; }"
              "%1::indicator:disabled {"
              "  border-color: %11;"
              "  background-color: %12;"
              "}"
              "%1::indicator:checked:disabled {"
              "  background: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5,"
              "    fx: 0.5, fy: 0.5, stop: 0 %10, stop: 0.45 %10,"
              "    stop: 0.5 %12, stop: 1 %12);"
              "}")
          .arg(selector)
          .arg(StyleHelper::toHexString(c.foreground))
          .arg(s.space2)
          .arg(s.space4 - 2)
          .arg(StyleHelper::toHexString(c.input))
          .arg(r.lg)
          .arg(StyleHelper::toHexString(c.background))
          .arg(StyleHelper::toHexString(c.primary))
          .arg(StyleHelper::toHexString(c.ring))
          .arg(StyleHelper::toHexString(c.mutedForeground))
          .arg(StyleHelper::toHexString(c.border))
          .arg(StyleHelper::toHexString(c.muted));

  for (auto *option : m_group->buttons()) {
    option->setFont(t.font(t.sizeBase));
    option->setStyleSheet(styles);
  }
}

} // namespace ui
