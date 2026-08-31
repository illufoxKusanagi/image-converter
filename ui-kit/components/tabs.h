#ifndef UI_TABS_H
#define UI_TABS_H

#include "../theme/theme.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

namespace ui {

enum class TabVariant {
  Underline,  // Minimal Linear/VS Code style with active bottom indicator
  Segmented   // Crisp desktop segmented bar with micro-borders
};

class Tabs : public QWidget {
  Q_OBJECT

public:
  explicit Tabs(QWidget *parent = nullptr);
  explicit Tabs(TabVariant variant, QWidget *parent = nullptr);
  ~Tabs() override = default;

  TabVariant variant() const;
  void setVariant(TabVariant variant);

  int addTab(const QString &text, const QIcon &icon = QIcon(), QWidget *page = nullptr);
  int insertTab(int index, const QString &text, const QIcon &icon = QIcon(), QWidget *page = nullptr);
  void removeTab(int index);

  int currentIndex() const;
  void setCurrentIndex(int index);

  int count() const;
  QString tabText(int index) const;
  void setTabText(int index, const QString &text);

  QWidget *widget(int index) const;

signals:
  void currentChanged(int index);

private slots:
  void applyThemeStyles();
  void onTabButtonClicked(int index);

private:
  TabVariant m_variant{TabVariant::Underline};
  int m_currentIndex{-1};

  QVBoxLayout *m_rootLayout;
  QWidget *m_tabBarWidget;
  QHBoxLayout *m_tabBarLayout;
  QStackedWidget *m_stackedWidget;

  struct TabItem {
    QString text;
    QIcon icon;
    QPushButton *button{nullptr};
    QWidget *page{nullptr};
  };
  QVector<TabItem> m_tabs;

  void rebuildTabBar();
};

} // namespace ui

#endif // UI_TABS_H

