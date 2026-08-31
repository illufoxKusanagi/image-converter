#include "sidebar.h"
#include "../theme/animation_helper.h"
#include "../theme/style_helper.h"

namespace ui {

Sidebar::Sidebar(QWidget *parent)
    : QFrame(parent) {
  setFixedWidth(220);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  m_rootLayout = new QVBoxLayout(this);
  m_rootLayout->setContentsMargins(8, 12, 8, 12);
  m_rootLayout->setSpacing(12);

  // Header
  m_headerWidget = new QWidget(this);
  m_headerLayout = new QHBoxLayout(m_headerWidget);
  m_headerLayout->setContentsMargins(4, 0, 4, 0);
  m_headerLayout->setSpacing(8);

  m_logoLabel = new QLabel(this);
  m_logoLabel->setVisible(false);

  m_titleLabel = new QLabel(this);
  m_titleLabel->setVisible(false);

  m_toggleButton = new QPushButton("☰", this);
  m_toggleButton->setFixedSize(32, 32);
  m_toggleButton->setCursor(Qt::PointingHandCursor);
  m_toggleButton->setToolTip("Toggle Sidebar");

  connect(m_toggleButton, &QPushButton::clicked, this, &Sidebar::onToggleClicked);

  m_headerLayout->addWidget(m_logoLabel);
  m_headerLayout->addWidget(m_titleLabel, 1);
  m_headerLayout->addWidget(m_toggleButton);
  m_rootLayout->addWidget(m_headerWidget);

  // Items scroll area
  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  m_itemsContainer = new QWidget(m_scrollArea);
  m_itemsLayout = new QVBoxLayout(m_itemsContainer);
  m_itemsLayout->setContentsMargins(0, 0, 0, 0);
  m_itemsLayout->setSpacing(4);
  m_itemsLayout->setAlignment(Qt::AlignTop);

  m_scrollArea->setWidget(m_itemsContainer);
  m_rootLayout->addWidget(m_scrollArea, 1);

  // Animation
  m_animation = new QVariantAnimation(this);
  m_animation->setDuration(AnimationHelper::DurationNormal);
  m_animation->setEasingCurve(AnimationHelper::standardEasing());

  connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
    setSidebarWidth(val.toInt());
  });

  connect(&Theme::instance(), &Theme::themeChanged, this, &Sidebar::applyThemeStyles);
  applyThemeStyles();
}

bool Sidebar::isCollapsed() const {
  return m_isCollapsed;
}

void Sidebar::setCollapsed(bool collapsed, bool animated) {
  if (m_isCollapsed == collapsed) {
    return;
  }
  m_isCollapsed = collapsed;

  int targetWidth = m_isCollapsed ? 56 : 220;

  if (animated) {
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(targetWidth);
    connect(m_animation, &QVariantAnimation::finished, this, [this]() {
      updateItemVisuals();
    });
    m_animation->start();
  } else {
    setSidebarWidth(targetWidth);
    updateItemVisuals();
  }

  emit collapsedChanged(m_isCollapsed);
}

void Sidebar::toggleCollapsed() {
  setCollapsed(!m_isCollapsed);
}

void Sidebar::setTitle(const QString &title, const QIcon &icon) {
  if (!icon.isNull()) {
    m_logoLabel->setPixmap(icon.pixmap(20, 20));
    m_logoLabel->setVisible(true);
  }
  m_titleLabel->setText(title);
  m_titleLabel->setVisible(!title.isEmpty() && !m_isCollapsed);
}

int Sidebar::addItem(const QString &text, const QIcon &icon, int id) {
  int index = m_items.size();
  int itemId = (id != -1) ? id : index;

  QPushButton *btn = new QPushButton(m_itemsContainer);
  btn->setFixedHeight(40);
  btn->setCursor(Qt::PointingHandCursor);
  btn->setIconSize(QSize(18, 18));
  if (!icon.isNull()) {
    btn->setIcon(icon);
  }

  SidebarItem item;
  item.id = itemId;
  item.text = text;
  item.icon = icon;
  item.button = btn;

  m_items.append(item);
  m_itemsLayout->addWidget(btn);

  connect(btn, &QPushButton::clicked, this, [this, index, itemId]() {
    setCurrentIndex(index);
    emit itemSelected(itemId, index);
  });

  if (m_currentIndex == -1) {
    setCurrentIndex(0);
  } else {
    updateItemVisuals();
  }

  return index;
}

void Sidebar::setCurrentIndex(int index) {
  if (index < 0 || index >= m_items.size()) {
    return;
  }
  m_currentIndex = index;
  updateItemVisuals();
}

int Sidebar::currentIndex() const {
  return m_currentIndex;
}

int Sidebar::sidebarWidth() const {
  return m_sidebarWidth;
}

void Sidebar::setSidebarWidth(int width) {
  m_sidebarWidth = width;
  setFixedWidth(width);
}

void Sidebar::setFooterWidget(QWidget *widget) {
  if (m_footerWidget) {
    m_rootLayout->removeWidget(m_footerWidget);
    m_footerWidget->setParent(nullptr);
  }
  m_footerWidget = widget;
  if (m_footerWidget) {
    m_rootLayout->addWidget(m_footerWidget);
  }
}

void Sidebar::onToggleClicked() {
  toggleCollapsed();
}

void Sidebar::updateItemVisuals() {
  m_titleLabel->setVisible(!m_isCollapsed && !m_titleLabel->text().isEmpty());

  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  for (int i = 0; i < m_items.size(); ++i) {
    QPushButton *btn = m_items[i].button;
    bool active = (i == m_currentIndex);

    if (m_isCollapsed) {
      btn->setText("");
      btn->setToolTip(m_items[i].text);
    } else {
      btn->setText(m_items[i].text);
      btn->setToolTip("");
    }

    QString bg = active ? StyleHelper::toHexString(c.accent) : "transparent";
    QString fg = active ? StyleHelper::toHexString(c.foreground) : StyleHelper::toHexString(c.mutedForeground);
    QString borderLeft = active ? QString("3px solid %1").arg(StyleHelper::toHexString(c.primary)) : "3px solid transparent";

    btn->setFont(t.font(t.sizeSm, active ? QFont::DemiBold : QFont::Normal));
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-left: %3;"
        "  border-radius: %4px;"
        "  text-align: %5;"
        "  padding: 0px %6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %7;"
        "  color: %8;"
        "}")
        .arg(bg)
        .arg(fg)
        .arg(borderLeft)
        .arg(r.sm)
        .arg(m_isCollapsed ? "center" : "left")
        .arg(m_isCollapsed ? 0 : 12)
        .arg(StyleHelper::toHexString(c.accent))
        .arg(StyleHelper::toHexString(c.foreground)));
  }
}

void Sidebar::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &t = Theme::instance().typography();

  setStyleSheet(QString(
      "ui--Sidebar, QFrame {"
      "  background-color: %1;"
      "  border-right: 1px solid %2;"
      "}")
      .arg(StyleHelper::toHexString(c.card))
      .arg(StyleHelper::toHexString(c.border)));

  m_titleLabel->setFont(t.font(t.sizeBase, QFont::Bold));
  m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(StyleHelper::toHexString(c.foreground)));

  m_toggleButton->setStyleSheet(QString(
      "QPushButton {"
      "  background-color: transparent;"
      "  color: %1;"
      "  border: none;"
      "  font-size: 14px;"
      "  border-radius: 4px;"
      "}"
      "QPushButton:hover {"
      "  background-color: %2;"
      "}")
      .arg(StyleHelper::toHexString(c.mutedForeground))
      .arg(StyleHelper::toHexString(c.accent)));

  m_scrollArea->setStyleSheet("background: transparent; border: none;");
  m_itemsContainer->setStyleSheet("background: transparent; border: none;");

  updateItemVisuals();
}

} // namespace ui

