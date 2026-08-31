#include "card.h"
#include "../theme/style_helper.h"

namespace ui {

Card::Card(QWidget *parent)
    : Card("", "", parent) {}

Card::Card(const QString &title, const QString &description, QWidget *parent)
    : QFrame(parent) {
  m_rootLayout = new QVBoxLayout(this);
  m_rootLayout->setContentsMargins(24, 24, 24, 24);
  m_rootLayout->setSpacing(16);

  m_headerLayout = new QVBoxLayout();
  m_headerLayout->setContentsMargins(0, 0, 0, 0);
  m_headerLayout->setSpacing(4);

  m_titleLabel = new QLabel(this);
  m_titleLabel->setVisible(false);

  m_descLabel = new QLabel(this);
  m_descLabel->setWordWrap(true);
  m_descLabel->setVisible(false);

  m_headerLayout->addWidget(m_titleLabel);
  m_headerLayout->addWidget(m_descLabel);
  m_rootLayout->addLayout(m_headerLayout);

  m_contentLayout = new QVBoxLayout();
  m_contentLayout->setContentsMargins(0, 0, 0, 0);
  m_contentLayout->setSpacing(12);
  m_rootLayout->addLayout(m_contentLayout, 1);

  m_footerLayout = new QHBoxLayout();
  m_footerLayout->setContentsMargins(0, 0, 0, 0);
  m_footerLayout->setSpacing(12);
  m_rootLayout->addLayout(m_footerLayout);

  if (!title.isEmpty()) {
    setTitle(title);
  }
  if (!description.isEmpty()) {
    setDescription(description);
  }

  connect(&Theme::instance(), &Theme::themeChanged, this, &Card::applyThemeStyles);
  applyThemeStyles();
}

void Card::setTitle(const QString &title) {
  m_titleLabel->setText(title);
  m_titleLabel->setVisible(!title.isEmpty());
}

void Card::setDescription(const QString &description) {
  m_descLabel->setText(description);
  m_descLabel->setVisible(!description.isEmpty());
}

void Card::setContentWidget(QWidget *widget) {
  if (widget) {
    m_contentLayout->addWidget(widget);
  }
}

void Card::setContentLayout(QLayout *layout) {
  if (layout) {
    m_contentLayout->addLayout(layout);
  }
}

void Card::setFooterWidget(QWidget *widget) {
  if (widget) {
    m_footerLayout->addWidget(widget);
  }
}

void Card::setFooterLayout(QLayout *layout) {
  if (layout) {
    m_footerLayout->addLayout(layout);
  }
}

QVBoxLayout *Card::contentLayout() const {
  return m_contentLayout;
}

QHBoxLayout *Card::footerLayout() const {
  return m_footerLayout;
}

void Card::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  m_titleLabel->setFont(t.font(t.sizeXl, QFont::DemiBold));
  m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(StyleHelper::toHexString(c.cardForeground)));

  m_descLabel->setFont(t.font(t.sizeSm, QFont::Normal));
  m_descLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(StyleHelper::toHexString(c.mutedForeground)));

  setStyleSheet(QString(
      "ui--Card, QFrame {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: %3px;"
      "}")
      .arg(StyleHelper::toHexString(c.card))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.lg));
}

} // namespace ui
