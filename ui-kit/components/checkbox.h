#ifndef UI_CHECKBOX_H
#define UI_CHECKBOX_H

#include <QCheckBox>

namespace ui {

class Checkbox : public QCheckBox {
  Q_OBJECT

public:
  explicit Checkbox(QWidget *parent = nullptr);
  explicit Checkbox(const QString &text, QWidget *parent = nullptr);

  bool hasError() const;
  void setError(bool hasError);

protected:
  void paintEvent(QPaintEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  bool m_hasError{false};
};

} // namespace ui

#endif // UI_CHECKBOX_H
