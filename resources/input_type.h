#ifndef INPUTTYPE_H
#define INPUTTYPE_H

#include <QString>
#include <optional>

class InputType {
public:
  QString type;
  QString label;
  QString unit;
  double value;
  bool isReadOnly;

  InputType(const QString &inputType, const QString &inputLabel,
            const QString &inputUnit = "", double value = 0.0,
            bool isReadOnly = false)
      : type(inputType), label(inputLabel), unit(inputUnit),
        isReadOnly(isReadOnly), value(value) {}
};

#endif // INPUTTYPE_H
