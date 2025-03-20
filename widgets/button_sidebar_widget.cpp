#include "button_sidebar_widget.h"

ButtonSidebarWidget::ButtonSidebarWidget(QWidget *parent,
                                         const QString &iconType,
                                         const QString &buttonLabel)
    : QWidget(parent) {
  mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(16);
  m_iconLabel = new QLabel(this);
  m_iconType = iconType;
  m_fixedHeight = MINIMUM_SIZE;
  m_fixedWidth = MINIMUM_SIZE;
  updateIcon(m_isEnabled);
  m_iconLabel->setMinimumSize(MINIMUM_SIZE, MINIMUM_SIZE);
  m_iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_textLabel = new QLabel(buttonLabel, this);
  m_textLabel->setStyleSheet(TextStyle::BodyBigBold() +
                             "color: " + Colors::StandardWhite.name() + ";");
  mainLayout->addWidget(m_iconLabel);
  mainLayout->addWidget(m_textLabel, 1);
  setLayout(mainLayout);
  setMinimumHeight(40);
  setMaximumHeight(40);
  setStyleSheet("QWidget { background-color: transparent; }");
}

void ButtonSidebarWidget::setIconSize(int width, int height) {
  m_fixedWidth = width;
  m_fixedHeight = height;
  m_iconLabel->setFixedSize(m_fixedWidth, m_fixedHeight);
}

void ButtonSidebarWidget::setLabelVisible(bool visible) {
  if (m_textLabel) {
    m_textLabel->setVisible(visible);
    if (mainLayout) {
      mainLayout->setSpacing(visible ? 16 : 0);
    }
  }
}

void ButtonSidebarWidget::setIconVisible(bool visible) {
  if (m_iconLabel) {
    m_iconLabel->setVisible(visible);
    if (mainLayout) {
      mainLayout->setSpacing(visible ? 16 : 0);
      mainLayout->setAlignment(m_textLabel,
                               visible ? Qt::AlignLeft : Qt::AlignCenter);
    }
  }
}

void ButtonSidebarWidget::updateAppearance(bool isShown) {
  static QString originalIconType = m_iconType;
  m_iconType = isShown ? originalIconType : originalIconType + "Collapsed";
  updateIcon(m_isEnabled);
}

void ButtonSidebarWidget::updateIcon(bool isEnabled) {
  QString iconPath = IconPaths::getIconPath(
      m_isEnabled ? m_iconType : m_iconType + "Disabled");
  QPixmap icon(iconPath);
  m_iconLabel->setPixmap(icon.scaled(m_fixedWidth, m_fixedHeight,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
}

void ButtonSidebarWidget::setEnabled(bool isEnabled) {
  m_textLabel->setStyleSheet(
      TextStyle::BodyBigBold() + "color: " +
      (isEnabled ? Colors::StandardWhite : Colors::Grey400).name() + ";");
  m_isEnabled = isEnabled;
  updateIcon(m_isEnabled);
}
