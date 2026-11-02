#ifndef UI_NUMBER_INPUT_H
#define UI_NUMBER_INPUT_H

#include <QDoubleSpinBox>
#include <QSpinBox>

namespace ui {

class NumberInput : public QSpinBox {
  Q_OBJECT

public:
  explicit NumberInput(QWidget *parent = nullptr);

  bool hasError() const;
  void setError(bool hasError);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  bool m_hasError{false};
};

class DecimalInput : public QDoubleSpinBox {
  Q_OBJECT

public:
  explicit DecimalInput(QWidget *parent = nullptr);

  bool hasError() const;
  void setError(bool hasError);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  bool m_hasError{false};
};

} // namespace ui

#endif // UI_NUMBER_INPUT_H
