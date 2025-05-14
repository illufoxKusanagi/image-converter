#ifndef INPUTFIELD_H
#define INPUTFIELD_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <optional>

/**
 * @brief A QWidget-based input field for numeric values with optional unit display.
 *
 * Provides a line edit for user input, an optional unit label, and emits a signal when the value changes.
 *
 * @param parent Optional parent widget.
 * @param unitText Optional text to display as the unit label next to the input field.
 */
class InputField : public QWidget {
  Q_OBJECT

signals:
  void valueChanged(double value);

public:
  explicit InputField(QWidget *parent = nullptr, const QString &unitText = "");
  void setValue(double value);
  void setPlaceholder();
  void setReadOnly(bool isReadOnly);
  void connectTextChanged();
  double getValue();
  bool isModified() const;
  void setModified(bool modified);

private:
  QLineEdit *m_input;
  QLabel *m_unitLabel;
};

#endif
