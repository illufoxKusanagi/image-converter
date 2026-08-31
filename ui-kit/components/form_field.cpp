#include "form_field.h"
#include "../theme/style_helper.h"

namespace ui {

FormField::FormField(QWidget *parent)
    : FormField("", nullptr, parent) {}

FormField::FormField(const QString &label, QWidget *control, QWidget *parent)
    : QWidget(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(0, 0, 0, 0);
  m_layout->setSpacing(6);

  m_labelLayout = new QHBoxLayout();
  m_labelLayout->setContentsMargins(0, 0, 0, 0);
  m_labelLayout->setSpacing(4);

  m_label = new QLabel(this);
  m_label->setVisible(false);

  m_requiredIndicator = new QLabel("*", this);
  m_requiredIndicator->setVisible(false);

  m_labelLayout->addWidget(m_label);
  m_labelLayout->addWidget(m_requiredIndicator);
  m_labelLayout->addStretch();
  m_layout->addLayout(m_labelLayout);

  m_descLabel = new QLabel(this);
  m_descLabel->setWordWrap(true);
  m_descLabel->setVisible(false);

  m_errorLabel = new QLabel(this);
  m_errorLabel->setWordWrap(true);
  m_errorLabel->setVisible(false);

  if (!label.isEmpty()) {
    setLabel(label);
  }
  if (control) {
    setControl(control);
  }

  m_layout->addWidget(m_descLabel);
  m_layout->addWidget(m_errorLabel);

  connect(&Theme::instance(), &Theme::themeChanged, this, &FormField::applyThemeStyles);
  applyThemeStyles();
}

QString FormField::label() const {
  return m_label->text();
}

void FormField::setLabel(const QString &label) {
  m_label->setText(label);
  m_label->setVisible(!label.isEmpty());
}

bool FormField::isRequired() const {
  return m_isRequired;
}

void FormField::setRequired(bool required) {
  m_isRequired = required;
  m_requiredIndicator->setVisible(required && !m_label->text().isEmpty());
}

void FormField::setDescription(const QString &description) {
  m_descLabel->setText(description);
  m_descLabel->setVisible(!description.isEmpty() && !m_hasError);
}

QString FormField::description() const {
  return m_descLabel->text();
}

void FormField::setError(const QString &errorMessage) {
  m_hasError = !errorMessage.isEmpty();
  m_errorLabel->setText(errorMessage);
  m_errorLabel->setVisible(m_hasError);

  if (m_hasError) {
    m_descLabel->setVisible(false);
  } else {
    m_descLabel->setVisible(!m_descLabel->text().isEmpty());
  }
}

void FormField::clearError() {
  setError("");
}

bool FormField::hasError() const {
  return m_hasError;
}

QString FormField::error() const {
  return m_errorLabel->text();
}

void FormField::setControl(QWidget *control) {
  if (m_control == control) {
    return;
  }
  if (m_control) {
    m_layout->removeWidget(m_control);
    m_control->setParent(nullptr);
  }
  m_control = control;
  if (m_control) {
    m_layout->insertWidget(1, m_control);
  }
}

QWidget *FormField::control() const {
  return m_control;
}

void FormField::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &t = Theme::instance().typography();

  m_label->setFont(t.font(t.sizeSm, QFont::DemiBold));
  m_label->setStyleSheet(QString("color: %1; background: transparent;")
                             .arg(StyleHelper::toHexString(c.foreground)));

  m_requiredIndicator->setFont(t.font(t.sizeSm, QFont::Bold));
  m_requiredIndicator->setStyleSheet(QString("color: %1; background: transparent;")
                                         .arg(StyleHelper::toHexString(c.destructive)));

  m_descLabel->setFont(t.font(t.sizeXs, QFont::Normal));
  m_descLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(StyleHelper::toHexString(c.mutedForeground)));

  m_errorLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
  m_errorLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(StyleHelper::toHexString(c.destructive)));
}

} // namespace ui
