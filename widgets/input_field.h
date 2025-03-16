#ifndef INPUTFIELD_H
#define INPUTFIELD_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <optional>

class InputField : public QWidget {
  Q_OBJECT

signals:
  void valueChanged(double value);

public:
  explicit InputField(const QString &unitText = "", QWidget *parent = nullptr);
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
