#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "../theme/theme.h"
#include "button.h"
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace ui {

class Dialog : public QDialog {
  Q_OBJECT

public:
  explicit Dialog(QWidget *parent = nullptr);
  Dialog(const QString &title, const QString &description = "", QWidget *parent = nullptr);
  ~Dialog() override = default;

  void setTitle(const QString &title);
  void setDescription(const QString &description);

  void setContentWidget(QWidget *widget);
  void setContentLayout(QLayout *layout);

  void setConfirmText(const QString &text);
  void setCancelText(const QString &text);
  void setDestructive(bool destructive);

  Button *confirmButton() const;
  Button *cancelButton() const;

  static bool confirm(QWidget *parent, const QString &title, const QString &description, const QString &confirmText = "Confirm", bool isDestructive = false);
  static void info(QWidget *parent, const QString &title, const QString &description);

private slots:
  void applyThemeStyles();

private:
  QVBoxLayout *m_rootLayout;
  QVBoxLayout *m_headerLayout;
  QLabel *m_titleLabel;
  QLabel *m_descLabel;
  QVBoxLayout *m_contentLayout;
  QHBoxLayout *m_footerLayout;
  Button *m_cancelButton;
  Button *m_confirmButton;
};

} // namespace ui

#endif // UI_DIALOG_H

