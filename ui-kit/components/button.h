#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "../theme/theme.h"
#include <QPushButton>

namespace ui {

enum class ButtonVariant {
  Primary,
  Secondary,
  Outline,
  Ghost,
  Destructive,
  Link
};

enum class ButtonSize {
  Small,
  Default,
  Large,
  Icon
};

class Button : public QPushButton {
  Q_OBJECT

public:
  explicit Button(QWidget *parent = nullptr);
  explicit Button(const QString &text, QWidget *parent = nullptr);
  Button(const QString &text, ButtonVariant variant, ButtonSize size = ButtonSize::Default, QWidget *parent = nullptr);
  ~Button() override = default;

  ButtonVariant variant() const;
  void setVariant(ButtonVariant variant);

  ButtonSize size() const;
  void setSize(ButtonSize size);

  bool isLoading() const;
  void setLoading(bool loading);

private slots:
  void applyThemeStyles();

private:
  ButtonVariant m_variant{ButtonVariant::Primary};
  ButtonSize m_size{ButtonSize::Default};
  bool m_isLoading{false};
  QString m_savedText;
};

} // namespace ui

#endif // UI_BUTTON_H
