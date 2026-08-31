#ifndef UI_TOAST_H
#define UI_TOAST_H

#include "../theme/theme.h"
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace ui {

enum class ToastVariant {
  Default,
  Success,
  Warning,
  Destructive
};

class Toast : public QFrame {
  Q_OBJECT

public:
  explicit Toast(QWidget *parent = nullptr);
  Toast(const QString &title, const QString &description = "", ToastVariant variant = ToastVariant::Default, QWidget *parent = nullptr);
  ~Toast() override = default;

  void setTitle(const QString &title);
  void setDescription(const QString &description);
  void setVariant(ToastVariant variant);
  void setDuration(int durationMs);

  void showToast();
  void dismiss();

  static Toast *show(QWidget *parent, const QString &title, const QString &description = "", ToastVariant variant = ToastVariant::Default, int durationMs = 3000);

protected:
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  ToastVariant m_variant{ToastVariant::Default};
  int m_durationMs{3000};

  QHBoxLayout *m_rootLayout;
  QLabel *m_iconLabel;
  QVBoxLayout *m_textLayout;
  QLabel *m_titleLabel;
  QLabel *m_descLabel;
  QPushButton *m_closeButton;

  QTimer *m_dismissTimer;
  QGraphicsOpacityEffect *m_opacityEffect;
  QVariantAnimation *m_anim;

  void setupPosition();
};

} // namespace ui

#endif // UI_TOAST_H

