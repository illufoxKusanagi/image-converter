#include "input_field.h"

/**
 * @brief Constructs an input field widget with an associated unit label.
 *
 * Creates a horizontally arranged widget containing a text input and a label displaying the specified unit text. Applies fixed sizing and custom styles to both elements.
 *
 * @param unitText The text to display as the unit label next to the input field.
 */
InputField::InputField(QWidget *parent, const QString &unitText)
    : QWidget(parent), m_input(new QLineEdit(this)),
      m_unitLabel(new QLabel(unitText, this)) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(m_input);
  layout->addWidget(m_unitLabel);

  m_input->setStyleSheet("padding: 4px 12px; border: 1px solid " +
                         Colors::Grey400.name() + "; border-radius: 8px;" +
                         TextStyle::BodySmallRegular() +
                         "color: " + Colors::Grey400.name() + ";");
  m_input->setFixedSize(128, 32);
  m_unitLabel->setStyleSheet(TextStyle::BodySmallRegular() +
                             "color: " + Colors::Grey700.name() + ";");
}

void InputField::setValue(double value) {
  if (value != 0)
    m_input->setText(QString::number(value));
  else
    m_input->clear();
  m_input->setModified(false);
}

double InputField::getValue() { return m_input->text().toDouble(); }

void InputField::setPlaceholder() {
  m_input->setPlaceholderText("enter value...");
}

void InputField::setReadOnly(bool isReadOnly) {
  m_input->setReadOnly(isReadOnly);
}

void InputField::connectTextChanged() {
  connect(m_input, &QLineEdit::textChanged, [this](const QString &text) {
    double value = text.toDouble();
    emit valueChanged(value);
  });
}

bool InputField::isModified() const { return m_input->isModified(); }

void InputField::setModified(bool modified) { m_input->setModified(modified); }
