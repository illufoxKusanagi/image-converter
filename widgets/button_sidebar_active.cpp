#include "button_sidebar_active.h"

ButtonSidebarActive::ButtonSidebarActive(const QString &iconType,
                                         const QString &buttonLabel,
                                         QWidget *parent)
    : QPushButton(parent), m_contentWidget(nullptr), m_layout(nullptr) {
  m_iconType = iconType;
  m_contentWidget = new ButtonSidebarWidget(m_iconType, buttonLabel, this);
  m_layout = new QHBoxLayout(this);
  m_layout->setContentsMargins(8, 8, 8, 8);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_layout->addWidget(m_contentWidget);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ButtonSidebarActive::setIconSize(int width, int height) {
  if (m_contentWidget) {
    m_contentWidget->setIconSize(width, height);
  }
}

void ButtonSidebarActive::setLabelVisible(bool visible) {
  if (m_contentWidget) {
    m_contentWidget->setLabelVisible(visible);
  }
}

void ButtonSidebarActive::setIconVisible(bool visible) {
  if (m_contentWidget) {
    m_contentWidget->setIconVisible(visible);
  }
}

void ButtonSidebarActive::updateIcon(bool isShown) {
  if (m_contentWidget) {
    m_contentWidget->updateAppearance(isShown);
  }
}

void ButtonSidebarActive::setEnabled(bool isEnabled) {
  QString buttonStyle = s_styleBase + s_styleHover + s_stylePressed;
  setStyleSheet(buttonStyle);
  m_contentWidget->setEnabled(isEnabled);
}

const QString ButtonSidebarActive::s_styleHover = "QPushButton:hover {"
                                                  "    background-color: " +
                                                  Colors::Secondary300.name() +
                                                  ";"
                                                  "}";

const QString ButtonSidebarActive::s_stylePressed =
    "QPushButton:pressed {"
    "    background-color: " +
    Colors::Secondary600.name() +
    ";"
    "}";

const QString ButtonSidebarActive::s_styleBase =
    "QPushButton {"
    "    padding: 8px 24px;"
    "    text-align: left;"
    "    border: 0 solid;"
    "    border-radius: 12px;"
    "    background-color: transparent;"
    "    min-height: 40px;"
    "    color: " +
    Colors::StandardWhite.name() +
    ";"
    "    " +
    TextStyle::BodyBigBold() + "}";
