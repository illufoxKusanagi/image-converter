#include "tabs.h"
#include "../theme/style_helper.h"

namespace ui {

Tabs::Tabs(QWidget *parent)
    : Tabs(TabVariant::Underline, parent) {}

Tabs::Tabs(TabVariant variant, QWidget *parent)
    : QWidget(parent), m_variant(variant) {
  m_rootLayout = new QVBoxLayout(this);
  m_rootLayout->setContentsMargins(0, 0, 0, 0);
  m_rootLayout->setSpacing(12);

  m_tabBarWidget = new QWidget(this);
  m_tabBarLayout = new QHBoxLayout(m_tabBarWidget);
  m_tabBarLayout->setContentsMargins(4, 4, 4, 4);
  m_tabBarLayout->setSpacing(4);
  m_tabBarLayout->setAlignment(Qt::AlignLeft);

  m_stackedWidget = new QStackedWidget(this);
  m_stackedWidget->setVisible(false);

  m_rootLayout->addWidget(m_tabBarWidget);
  m_rootLayout->addWidget(m_stackedWidget, 1);

  connect(&Theme::instance(), &Theme::themeChanged, this, &Tabs::applyThemeStyles);
  applyThemeStyles();
}

TabVariant Tabs::variant() const {
  return m_variant;
}

void Tabs::setVariant(TabVariant variant) {
  if (m_variant != variant) {
    m_variant = variant;
    applyThemeStyles();
  }
}

int Tabs::addTab(const QString &text, const QIcon &icon, QWidget *page) {
  return insertTab(m_tabs.size(), text, icon, page);
}

int Tabs::insertTab(int index, const QString &text, const QIcon &icon, QWidget *page) {
  int targetIndex = qBound(0, index, m_tabs.size());

  QPushButton *btn = new QPushButton(text, m_tabBarWidget);
  if (!icon.isNull()) {
    btn->setIcon(icon);
  }
  btn->setCursor(Qt::PointingHandCursor);

  TabItem item;
  item.text = text;
  item.icon = icon;
  item.button = btn;
  item.page = page;

  m_tabs.insert(targetIndex, item);
  m_tabBarLayout->insertWidget(targetIndex, btn);

  QWidget *pageWidget = page;
  if (!pageWidget) {
    pageWidget = new QWidget(m_stackedWidget);
  }
  m_stackedWidget->insertWidget(targetIndex, pageWidget);

  connect(btn, &QPushButton::clicked, this, [this, btn]() {
    for (int i = 0; i < m_tabs.size(); ++i) {
      if (m_tabs[i].button == btn) {
        setCurrentIndex(i);
        break;
      }
    }
  });

  if (m_currentIndex == -1 || m_tabs.size() == 1) {
    setCurrentIndex(0);
  } else {
    applyThemeStyles();
  }

  return targetIndex;
}

void Tabs::removeTab(int index) {
  if (index < 0 || index >= m_tabs.size()) {
    return;
  }

  TabItem item = m_tabs.takeAt(index);
  m_tabBarLayout->removeWidget(item.button);
  item.button->deleteLater();

  QWidget *stackedPage = m_stackedWidget->widget(index);
  if (stackedPage) {
    m_stackedWidget->removeWidget(stackedPage);
    if (!item.page) {
      stackedPage->deleteLater();
    }
  }

  if (m_tabs.isEmpty()) {
    m_currentIndex = -1;
    m_stackedWidget->setVisible(false);
  } else {
    int newIndex = qBound(0, m_currentIndex, m_tabs.size() - 1);
    setCurrentIndex(newIndex);
  }
}

int Tabs::currentIndex() const {
  return m_currentIndex;
}

void Tabs::setCurrentIndex(int index) {
  if (index < 0 || index >= m_tabs.size()) {
    return;
  }

  m_currentIndex = index;
  if (m_stackedWidget->count() > index) {
    m_stackedWidget->setCurrentIndex(index);
  }
  m_stackedWidget->setVisible(m_tabs[index].page != nullptr);

  applyThemeStyles();
  emit currentChanged(m_currentIndex);
}

int Tabs::count() const {
  return m_tabs.size();
}

QString Tabs::tabText(int index) const {
  if (index >= 0 && index < m_tabs.size()) {
    return m_tabs[index].text;
  }
  return QString();
}

void Tabs::setTabText(int index, const QString &text) {
  if (index >= 0 && index < m_tabs.size()) {
    m_tabs[index].text = text;
    m_tabs[index].button->setText(text);
  }
}

QWidget *Tabs::widget(int index) const {
  if (index >= 0 && index < m_tabs.size()) {
    return m_tabs[index].page;
  }
  return nullptr;
}

void Tabs::onTabButtonClicked(int index) {
  setCurrentIndex(index);
}

void Tabs::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  if (m_variant == TabVariant::Segmented) {
    m_tabBarLayout->setContentsMargins(3, 3, 3, 3);
    m_tabBarLayout->setSpacing(2);
    m_tabBarWidget->setStyleSheet(QString(
        "background-color: %1;"
        "border: 1px solid %2;"
        "border-radius: %3px;")
        .arg(StyleHelper::toHexString(c.secondary))
        .arg(StyleHelper::toHexString(c.border))
        .arg(r.sm));

    for (int i = 0; i < m_tabs.size(); ++i) {
      bool active = (i == m_currentIndex);
      QString bg = active ? StyleHelper::toHexString(c.background) : "transparent";
      QString fg = active ? StyleHelper::toHexString(c.foreground) : StyleHelper::toHexString(c.mutedForeground);
      QString border = active ? QString("1px solid %1").arg(StyleHelper::toHexString(c.border)) : "1px solid transparent";

      m_tabs[i].button->setFont(t.font(t.sizeSm, active ? QFont::DemiBold : QFont::Normal));
      m_tabs[i].button->setStyleSheet(QString(
          "QPushButton {"
          "  background-color: %1;"
          "  color: %2;"
          "  border: %3;"
          "  border-radius: %4px;"
          "  padding: 5px 14px;"
          "}"
          "QPushButton:hover {"
          "  color: %5;"
          "}")
          .arg(bg)
          .arg(fg)
          .arg(border)
          .arg(r.sm > 2 ? r.sm - 1 : 2)
          .arg(StyleHelper::toHexString(c.foreground)));
    }
  } else {
    // Underline Variant (Linear / VS Code style)
    m_tabBarLayout->setContentsMargins(0, 0, 0, 0);
    m_tabBarLayout->setSpacing(8);
    m_tabBarWidget->setStyleSheet(QString(
        "background-color: transparent;"
        "border-bottom: 1px solid %1;")
        .arg(StyleHelper::toHexString(c.border)));

    for (int i = 0; i < m_tabs.size(); ++i) {
      bool active = (i == m_currentIndex);
      QString fg = active ? StyleHelper::toHexString(c.foreground) : StyleHelper::toHexString(c.mutedForeground);
      QString borderBottom = active ? QString("2px solid %1").arg(StyleHelper::toHexString(c.primary)) : "2px solid transparent";

      m_tabs[i].button->setFont(t.font(t.sizeSm, active ? QFont::DemiBold : QFont::Normal));
      m_tabs[i].button->setStyleSheet(QString(
          "QPushButton {"
          "  background-color: transparent;"
          "  color: %1;"
          "  border: none;"
          "  border-bottom: %2;"
          "  border-radius: 0px;"
          "  padding: 8px 16px;"
          "}"
          "QPushButton:hover {"
          "  color: %3;"
          "}")
          .arg(fg)
          .arg(borderBottom)
          .arg(StyleHelper::toHexString(c.foreground)));
    }
  }
}

} // namespace ui

