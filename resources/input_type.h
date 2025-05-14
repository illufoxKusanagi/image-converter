#ifndef INPUTTYPE_H
#define INPUTTYPE_H

#include <QString>
#include <optional>

/**
 * @brief Constructs an InputType object with default values.
 *
 * Initializes type, label, and unit as empty strings, value as 0.0, and isReadOnly as false.
 */
 
/**
 * @brief Constructs an InputType object with specified values.
 *
 * @param inputType The type of the input element.
 * @param inputLabel The label for the input element.
 * @param inputUnit The unit associated with the input element (optional).
 * @param value The value of the input element (optional, defaults to 0.0).
 * @param isReadOnly Whether the input is read-only (optional, defaults to false).
 */
class InputType {
public:
  QString type;
  QString label;
  QString unit;
  double value;
  bool isReadOnly;
  InputType() : type(""), label(""), unit(""), isReadOnly(false) {}
  InputType(const QString &inputType, const QString &inputLabel,
            const QString &inputUnit = "", double value = 0.0,
            bool isReadOnly = false)
      : type(inputType), label(inputLabel), unit(inputUnit),
        isReadOnly(isReadOnly), value(value) {}
};

#endif // INPUTTYPE_H
