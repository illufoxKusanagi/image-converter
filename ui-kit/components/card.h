#ifndef UI_CARD_H
#define UI_CARD_H

#include "../theme/theme.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace ui {

class Card : public QFrame {
  Q_OBJECT

public:
  explicit Card(QWidget *parent = nullptr);
  Card(const QString &title, const QString &description = "", QWidget *parent = nullptr);
  ~Card() override = default;

  void setTitle(const QString &title);
  void setDescription(const QString &description);

  void setContentWidget(QWidget *widget);
  void setContentLayout(QLayout *layout);

  void setFooterWidget(QWidget *widget);
  void setFooterLayout(QLayout *layout);

  QVBoxLayout *contentLayout() const;
  QHBoxLayout *footerLayout() const;

private slots:
  void applyThemeStyles();

private:
  QVBoxLayout *m_rootLayout;
  QVBoxLayout *m_headerLayout;
  QLabel *m_titleLabel;
  QLabel *m_descLabel;
  QVBoxLayout *m_contentLayout;
  QHBoxLayout *m_footerLayout;
};

} // namespace ui

#endif // UI_CARD_H
