#ifndef UI_FORM_FIELD_H
#define UI_FORM_FIELD_H

#include "../theme/theme.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {

class FormField : public QWidget {
  Q_OBJECT

public:
  explicit FormField(QWidget *parent = nullptr);
  explicit FormField(const QString &label, QWidget *control = nullptr, QWidget *parent = nullptr);
  ~FormField() override = default;

  QString label() const;
  void setLabel(const QString &label);

  bool isRequired() const;
  void setRequired(bool required);

  void setDescription(const QString &description);
  QString description() const;

  void setError(const QString &errorMessage);
  void clearError();
  bool hasError() const;
  QString error() const;

  void setControl(QWidget *control);
  QWidget *control() const;

private slots:
  void applyThemeStyles();

private:
  QVBoxLayout *m_layout;
  QHBoxLayout *m_labelLayout;
  QLabel *m_label;
  QLabel *m_requiredIndicator;
  QWidget *m_control{nullptr};
  QLabel *m_descLabel;
  QLabel *m_errorLabel;
  bool m_isRequired{false};
  bool m_hasError{false};
};

} // namespace ui

#endif // UI_FORM_FIELD_H
