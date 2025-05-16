#include "input_dropdown.h"

InputDropdown::InputDropdown(QWidget *parent, const QStringList options)
    : QWidget(parent), m_dropdown(new QComboBox(this)), m_options(options) {
  connect(m_dropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { emit valueChanged(); });
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  m_dropdown->addItems(options);
  m_dropdown->setFixedSize(128, 40);
  m_dropdown->setStyleSheet(
      "QComboBox {"
      "    color: " +
      Colors::StandardBlack.name() +
      ";"
      "    background-color: " +
      Colors::StandardWhite.name() +
      ";"
      "    border: 1px solid " +
      Colors::Primary500.name() +
      ";"
      "    border-radius: 8px;"
      "    padding: 2px 8px 2px 8px;"
      "    font-size: 14px;"
      "}"
      "QComboBox:hover {"
      "    border-color: " +
      Colors::Primary700.name() +
      ";"
      "}"
      "QComboBox:focus {"
      "    border-color: " +
      Colors::Primary700.name() +
      ";"
      "}"
      "QComboBox::drop-down {"
      "    subcontrol-origin: padding;"
      "    subcontrol-position: top right;"
      "    width: 32px;"
      "    border-left-width: 1px;"
      "    border-left-color: " +
      Colors::Primary500.name() +
      ";"
      "    border-left-style: solid;"
      "    border-top-right-radius: 8px;"
      "    border-bottom-right-radius: 8px;"
      "    background-color: " +
      Colors::Primary100.name() +
      ";"
      "}"
      "QComboBox::drop-down:hover {"
      "    background-color: " +
      Colors::Primary200.name() +
      ";"
      "}"
      "QComboBox::drop-down:pressed {"
      "    background-color: " +
      Colors::Primary300.name() +
      ";"
      "}"
      "QComboBox::down-arrow {"
      "    image: url(:/icons/icons/arrow-down-black.svg);"
      "    width: 10px;"
      "    height: 10px;"
      "}"
      "QComboBox QAbstractItemView {"
      "    border: 1px solid " +
      Colors::Primary500.name() +
      ";"
      "    background-color: " +
      Colors::StandardWhite.name() +
      ";"
      "    color: " +
      Colors::Secondary600.name() +
      ";"
      "    selection-background-color: " +
      Colors::Primary200.name() +
      ";"
      "    selection-color: " +
      Colors::Secondary700.name() +
      ";"
      "    padding: 2px;"
      "    border-radius: 4px;"
      "}"
      "QComboBox:disabled {"
      "    background-color: " +
      Colors::Grey100.name() +
      ";"
      "    color: " +
      Colors::Grey400.name() +
      ";"
      "    border-color: " +
      Colors::Grey300.name() +
      ";"
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
