#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "../theme/theme.h"
#include <QIcon>
#include <QPushButton>

namespace ui {

class Spinner;

enum class ButtonVariant {
  Primary,
  Secondary,
  Outline,
  Ghost,
  Destructive,
  Link
};

enum class ButtonSize { Small, Default, Large, Icon };

class Button : public QPushButton {
  Q_OBJECT

public:
  explicit Button(QWidget *parent = nullptr);
  explicit Button(const QString &text, QWidget *parent = nullptr);
  Button(const QString &text, ButtonVariant variant,
         ButtonSize size = ButtonSize::Default, QWidget *parent = nullptr);
  ~Button() override = default;

  ButtonVariant variant() const;
  void setVariant(ButtonVariant variant);

  ButtonSize size() const;
  void setSize(ButtonSize size);

  bool isLoading() const;
  void setLoading(bool loading);
  QString loadingText() const;
  void setLoadingText(const QString &text);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  ButtonVariant m_variant{ButtonVariant::Primary};
  ButtonSize m_size{ButtonSize::Default};
  bool m_isLoading{false};
  bool m_wasExplicitlyDisabled{false};
  QString m_loadingText;
  QString m_savedText;
  QIcon m_savedIcon;
  QSize m_savedSizeHint;
  QSize m_savedMinimumSizeHint;
  Spinner *m_loadingSpinner{nullptr};
};

} // namespace ui

#endif // UI_BUTTON_H
