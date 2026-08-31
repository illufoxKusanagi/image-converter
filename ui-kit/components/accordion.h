#ifndef UI_ACCORDION_H
#define UI_ACCORDION_H

#include "../theme/theme.h"
#include <QLabel>
#include <QPushButton>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {

class Accordion : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int contentHeight READ contentHeight WRITE setContentHeight)

public:
  explicit Accordion(QWidget *parent = nullptr);
  explicit Accordion(const QString &title, QWidget *content = nullptr, QWidget *parent = nullptr);
  ~Accordion() override = default;

  QString title() const;
  void setTitle(const QString &title);

  bool isExpanded() const;
  void setExpanded(bool expanded, bool animated = true);
  void toggleExpanded();

  void setContentWidget(QWidget *content);
  QWidget *contentWidget() const;

  int contentHeight() const;
  void setContentHeight(int height);

signals:
  void toggled(bool expanded);

private slots:
  void applyThemeStyles();
  void onHeaderClicked();

private:
  QVBoxLayout *m_rootLayout;
  QPushButton *m_headerButton;
  QHBoxLayout *m_headerLayout;
  QLabel *m_titleLabel;
  QLabel *m_chevronLabel;
  QWidget *m_contentContainer;
  QVBoxLayout *m_contentLayout;
  QWidget *m_contentWidget{nullptr};

  bool m_isExpanded{false};
  int m_contentHeight{0};
  QVariantAnimation *m_animation{nullptr};

  void updateChevron();
};

} // namespace ui

#endif // UI_ACCORDION_H

