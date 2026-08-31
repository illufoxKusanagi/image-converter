#include "accordion.h"
#include "../theme/animation_helper.h"
#include "../theme/style_helper.h"

namespace ui {

Accordion::Accordion(QWidget *parent)
    : Accordion("", nullptr, parent) {}

Accordion::Accordion(const QString &title, QWidget *content, QWidget *parent)
    : QWidget(parent) {
  m_rootLayout = new QVBoxLayout(this);
  m_rootLayout->setContentsMargins(0, 0, 0, 0);
  m_rootLayout->setSpacing(0);

  // Header button
  m_headerButton = new QPushButton(this);
  m_headerButton->setCursor(Qt::PointingHandCursor);
  m_headerButton->setFixedHeight(44);

  m_headerLayout = new QHBoxLayout(m_headerButton);
  m_headerLayout->setContentsMargins(12, 0, 12, 0);
  m_headerLayout->setSpacing(8);

  m_titleLabel = new QLabel(title, m_headerButton);
  m_chevronLabel = new QLabel("▼", m_headerButton);

  m_headerLayout->addWidget(m_titleLabel, 1);
  m_headerLayout->addWidget(m_chevronLabel, 0);

  connect(m_headerButton, &QPushButton::clicked, this, &Accordion::onHeaderClicked);

  // Content container
  m_contentContainer = new QWidget(this);
  m_contentLayout = new QVBoxLayout(m_contentContainer);
  m_contentLayout->setContentsMargins(12, 8, 12, 12);
  m_contentLayout->setSpacing(8);
  m_contentContainer->setFixedHeight(0);
  m_contentContainer->setVisible(false);

  m_rootLayout->addWidget(m_headerButton);
  m_rootLayout->addWidget(m_contentContainer);

  m_animation = new QVariantAnimation(this);
  m_animation->setDuration(AnimationHelper::DurationNormal);
  m_animation->setEasingCurve(AnimationHelper::standardEasing());

  connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &val) {
    setContentHeight(val.toInt());
  });

  if (content) {
    setContentWidget(content);
  }

  updateChevron();

  connect(&Theme::instance(), &Theme::themeChanged, this, &Accordion::applyThemeStyles);
  applyThemeStyles();
}

QString Accordion::title() const {
  return m_titleLabel->text();
}

void Accordion::setTitle(const QString &title) {
  m_titleLabel->setText(title);
}

bool Accordion::isExpanded() const {
  return m_isExpanded;
}

void Accordion::setExpanded(bool expanded, bool animated) {
  if (m_isExpanded == expanded) {
    return;
  }
  m_isExpanded = expanded;
  updateChevron();

  int targetHeight = 0;
  if (m_isExpanded) {
    m_contentContainer->setVisible(true);
    m_contentContainer->setMaximumHeight(16777215);
    targetHeight = m_contentLayout->sizeHint().height();
  }

  if (animated) {
    m_animation->stop();
    m_animation->setStartValue(m_contentContainer->height());
    m_animation->setEndValue(targetHeight);
    connect(m_animation, &QVariantAnimation::finished, this, [this]() {
      if (!m_isExpanded) {
        m_contentContainer->setVisible(false);
      } else {
        m_contentContainer->setMaximumHeight(16777215);
      }
    });
    m_animation->start();
  } else {
    setContentHeight(targetHeight);
    m_contentContainer->setVisible(m_isExpanded);
  }

  emit toggled(m_isExpanded);
}

void Accordion::toggleExpanded() {
  setExpanded(!m_isExpanded);
}

void Accordion::setContentWidget(QWidget *content) {
  if (m_contentWidget == content) {
    return;
  }
  if (m_contentWidget) {
    m_contentLayout->removeWidget(m_contentWidget);
    m_contentWidget->setParent(nullptr);
  }
  m_contentWidget = content;
  if (m_contentWidget) {
    m_contentLayout->addWidget(m_contentWidget);
  }
}

QWidget *Accordion::contentWidget() const {
  return m_contentWidget;
}

int Accordion::contentHeight() const {
  return m_contentHeight;
}

void Accordion::setContentHeight(int height) {
  m_contentHeight = height;
  m_contentContainer->setFixedHeight(height);
}

void Accordion::onHeaderClicked() {
  toggleExpanded();
}

void Accordion::updateChevron() {
  m_chevronLabel->setText(m_isExpanded ? "▲" : "▼");
}

void Accordion::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  m_titleLabel->setFont(t.font(t.sizeBase, QFont::DemiBold));
  m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(StyleHelper::toHexString(c.foreground)));

  m_chevronLabel->setFont(t.font(t.sizeSm, QFont::Normal));
  m_chevronLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                    .arg(StyleHelper::toHexString(c.mutedForeground)));

  m_headerButton->setStyleSheet(QString(
      "QPushButton {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: %3px;"
      "  text-align: left;"
      "}"
      "QPushButton:hover {"
      "  background-color: %4;"
      "}")
      .arg(StyleHelper::toHexString(c.card))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.md)
      .arg(StyleHelper::toHexString(c.accent)));

  m_contentContainer->setStyleSheet(QString(
      "background-color: %1;"
      "border-left: 1px solid %2;"
      "border-right: 1px solid %2;"
      "border-bottom: 1px solid %2;"
      "border-bottom-left-radius: %3px;"
      "border-bottom-right-radius: %3px;")
      .arg(StyleHelper::toHexString(c.card))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.md));
}

} // namespace ui

