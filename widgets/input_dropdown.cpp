#include "input_dropdown.h"

/**
 * @brief Constructs a styled dropdown widget populated with the given options.
 *
 * Initializes the dropdown menu with the provided list of options, applies custom visual styling, and sets up signal connections to emit a value change notification when the selection changes.
 *
 * @param options List of strings to display as selectable options in the dropdown.
 */
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

/**
 * @brief Sets the currently displayed text in the dropdown to the specified value.
 *
 * If the provided text matches one of the dropdown options, it becomes the selected item.
 */
void InputDropdown::setCurrentText(const QString &text) {
  m_dropdown->setCurrentText(text);
}

/**
 * @brief Placeholder method that currently performs no action.
 *
 * This method calls the combo box's currentIndex() but does not modify state or return a value.
 */
void InputDropdown::setValue() { m_dropdown->currentIndex(); }

/**
 * @brief Retrieves the current value of the dropdown selection.
 *
 * If any option in the dropdown contains alphabetic characters, returns the current index as a double.
 * Otherwise, returns the current text converted to a double.
 *
 * @return double The current index or the numeric value of the selected option, depending on the options' content.
 */
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
