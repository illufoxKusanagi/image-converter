#ifndef UI_EMPTY_STATE_H
#define UI_EMPTY_STATE_H

#include "button.h"
#include <QFrame>
#include <QIcon>
#include <QString>

class QLabel;
class QVBoxLayout;

namespace ui {

class EmptyState : public QFrame {
  Q_OBJECT

public:
  explicit EmptyState(QWidget *parent = nullptr);
  EmptyState(const QString &title, const QString &description,
             QWidget *parent = nullptr);

  void setTitle(const QString &title);
  QString title() const;

  void setDescription(const QString &description);
  QString description() const;

  void setIcon(const QIcon &icon);
  void setActionText(const QString &text);
  Button *actionButton() const;

signals:
  void actionTriggered();

private slots:
  void applyThemeStyles();

private:
  void updateIcon();

  QIcon m_icon;
  QVBoxLayout *m_layout{nullptr};
  QLabel *m_iconLabel{nullptr};
  QLabel *m_titleLabel{nullptr};
  QLabel *m_descriptionLabel{nullptr};
  Button *m_actionButton{nullptr};
};

} // namespace ui

#endif // UI_EMPTY_STATE_H
