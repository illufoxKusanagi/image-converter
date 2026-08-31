#include "dialog.h"
#include "../theme/style_helper.h"

namespace ui {

Dialog::Dialog(QWidget *parent)
    : Dialog("", "", parent) {}

Dialog::Dialog(const QString &title, const QString &description, QWidget *parent)
    : QDialog(parent) {
  setModal(true);
  setMinimumWidth(400);
  setMaximumWidth(520);

  m_rootLayout = new QVBoxLayout(this);
  m_rootLayout->setContentsMargins(24, 24, 24, 24);
  m_rootLayout->setSpacing(16);

  m_headerLayout = new QVBoxLayout();
  m_headerLayout->setContentsMargins(0, 0, 0, 0);
  m_headerLayout->setSpacing(6);

  m_titleLabel = new QLabel(title, this);
  m_descLabel = new QLabel(description, this);
  m_descLabel->setWordWrap(true);
  m_descLabel->setVisible(!description.isEmpty());

  m_headerLayout->addWidget(m_titleLabel);
  m_headerLayout->addWidget(m_descLabel);
  m_rootLayout->addLayout(m_headerLayout);

  m_contentLayout = new QVBoxLayout();
  m_contentLayout->setContentsMargins(0, 0, 0, 0);
  m_contentLayout->setSpacing(12);
  m_rootLayout->addLayout(m_contentLayout, 1);

  m_footerLayout = new QHBoxLayout();
  m_footerLayout->setContentsMargins(0, 0, 0, 0);
  m_footerLayout->setSpacing(8);
  m_footerLayout->addStretch();

  m_cancelButton = new Button("Cancel", ButtonVariant::Outline, ButtonSize::Default, this);
  m_confirmButton = new Button("Confirm", ButtonVariant::Primary, ButtonSize::Default, this);

  connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(m_confirmButton, &QPushButton::clicked, this, &QDialog::accept);

  m_footerLayout->addWidget(m_cancelButton);
  m_footerLayout->addWidget(m_confirmButton);
  m_rootLayout->addLayout(m_footerLayout);

  connect(&Theme::instance(), &Theme::themeChanged, this, &Dialog::applyThemeStyles);
  applyThemeStyles();
}

void Dialog::setTitle(const QString &title) {
  m_titleLabel->setText(title);
}

void Dialog::setDescription(const QString &description) {
  m_descLabel->setText(description);
  m_descLabel->setVisible(!description.isEmpty());
}

void Dialog::setContentWidget(QWidget *widget) {
  if (widget) {
    m_contentLayout->addWidget(widget);
  }
}

void Dialog::setContentLayout(QLayout *layout) {
  if (layout) {
    m_contentLayout->addLayout(layout);
  }
}

void Dialog::setConfirmText(const QString &text) {
  m_confirmButton->setText(text);
}

void Dialog::setCancelText(const QString &text) {
  m_cancelButton->setText(text);
}

void Dialog::setDestructive(bool destructive) {
  m_confirmButton->setVariant(destructive ? ButtonVariant::Destructive : ButtonVariant::Primary);
}

Button *Dialog::confirmButton() const {
  return m_confirmButton;
}

Button *Dialog::cancelButton() const {
  return m_cancelButton;
}

bool Dialog::confirm(QWidget *parent, const QString &title, const QString &description, const QString &confirmText, bool isDestructive) {
  Dialog dlg(title, description, parent);
  dlg.setConfirmText(confirmText);
  dlg.setDestructive(isDestructive);
  return dlg.exec() == QDialog::Accepted;
}

void Dialog::info(QWidget *parent, const QString &title, const QString &description) {
  Dialog dlg(title, description, parent);
  dlg.cancelButton()->setVisible(false);
  dlg.setConfirmText("OK");
  dlg.exec();
}

void Dialog::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  m_titleLabel->setFont(t.font(t.sizeLg, QFont::DemiBold));
  m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(StyleHelper::toHexString(c.foreground)));

  m_descLabel->setFont(t.font(t.sizeSm, QFont::Normal));
  m_descLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(StyleHelper::toHexString(c.mutedForeground)));

  setStyleSheet(QString(
      "QDialog {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: %3px;"
      "}")
      .arg(StyleHelper::toHexString(c.card))
      .arg(StyleHelper::toHexString(c.border))
      .arg(r.lg));
}

} // namespace ui

