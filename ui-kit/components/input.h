#ifndef UI_INPUT_H
#define UI_INPUT_H

#include "../theme/theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace ui {

class Input : public QWidget {
  Q_OBJECT

public:
  explicit Input(QWidget *parent = nullptr);
  explicit Input(const QString &placeholder, QWidget *parent = nullptr);
  ~Input() override = default;

  QString text() const;
  void setText(const QString &text);

  QString placeholderText() const;
  void setPlaceholderText(const QString &placeholder);

  void setPrefixText(const QString &prefix);
  void setSuffixText(const QString &suffix);

  bool isClearButtonEnabled() const;
  void setClearButtonEnabled(bool enable);

  bool isReadOnly() const;
  void setReadOnly(bool readOnly);

  bool hasError() const;
  void setError(bool hasError);

  QLineEdit *lineEdit() const;

signals:
  void textChanged(const QString &text);
  void textEdited(const QString &text);
  void returnPressed();

private slots:
  void applyThemeStyles();

private:
  QHBoxLayout *m_layout;
  QLineEdit *m_lineEdit;
  QLabel *m_prefixLabel;
  QLabel *m_suffixLabel;
  QPushButton *m_clearButton;
  bool m_hasError{false};
};

} // namespace ui

#endif // UI_INPUT_H
