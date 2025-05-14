#include "input_dropdown.h"

InputDropdown::InputDropdown(QWidget *parent, const QStringList options)
    : QWidget(parent), m_dropdown(new QComboBox(this)), m_options(options) {
  connect(m_dropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { emit valueChanged(); });
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  m_dropdown->addItems(options);
  m_dropdown->setFixedSize(128, 32);
  m_dropdown->setStyleSheet(
      "QComboBox {"
      "    padding: 4px 8px;"
      "    border: 1px solid " +
      Colors::Secondary400.name() +
      ";"
      "    border-radius: 8px;" +
      TextStyle::BodySmallRegular() +
      "    color: " + Colors::Secondary400.name() +
      ";"
      "}"
      "QComboBox::drop-down {"
      "    width: 16px;"
      "    border: none;"
      "    margin-right: 8px;"
      "}"
      "QComboBox::down-arrow {"
      "    color : " +
      Colors::Grey900.name() +
      ";"
      "    image: url(:/icons/icons/arrow-down-black.svg);"
      "    width: 16px;"
      "    height: 16px;"
      "    margin-right: 4px;"
      "}");

  layout->addWidget(m_dropdown);
}

void InputDropdown::setCurrentText(const QString &text) {
  m_dropdown->setCurrentText(text);
}

void InputDropdown::setValue() { m_dropdown->currentIndex(); }

double InputDropdown::getValue() {
  QRegularExpression regex("[A-Za-z]");
  bool hasLetter = false;
  for (const QString &option : m_options) {
    if (regex.match(option).hasMatch()) {
      hasLetter = true;
      break;
    }
  }
  return hasLetter ? m_dropdown->currentIndex()
                   : m_dropdown->currentText().toDouble();
}
