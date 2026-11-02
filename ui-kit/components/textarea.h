#ifndef UI_TEXTAREA_H
#define UI_TEXTAREA_H

#include <QPlainTextEdit>

namespace ui {

class Textarea : public QPlainTextEdit {
  Q_OBJECT

public:
  explicit Textarea(QWidget *parent = nullptr);
  explicit Textarea(const QString &placeholder, QWidget *parent = nullptr);

  bool hasError() const;
  void setError(bool hasError);

protected:
  bool event(QEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  bool m_hasError{false};
};

} // namespace ui

#endif // UI_TEXTAREA_H
