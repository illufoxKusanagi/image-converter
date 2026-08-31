#ifndef UI_SIDEBAR_H
#define UI_SIDEBAR_H

#include "../theme/theme.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QVector>

namespace ui {

class Sidebar : public QFrame {
  Q_OBJECT
  Q_PROPERTY(int sidebarWidth READ sidebarWidth WRITE setSidebarWidth)

public:
  explicit Sidebar(QWidget *parent = nullptr);
  ~Sidebar() override = default;

  bool isCollapsed() const;
  void setCollapsed(bool collapsed, bool animated = true);
  void toggleCollapsed();

  void setTitle(const QString &title, const QIcon &icon = QIcon());

  int addItem(const QString &text, const QIcon &icon = QIcon(), int id = -1);
  void setCurrentIndex(int index);
  int currentIndex() const;

  int sidebarWidth() const;
  void setSidebarWidth(int width);

  void setFooterWidget(QWidget *widget);

signals:
  void itemSelected(int id, int index);
  void collapsedChanged(bool isCollapsed);

private slots:
  void applyThemeStyles();
  void onToggleClicked();

private:
  bool m_isCollapsed{false};
  int m_sidebarWidth{220};
  int m_currentIndex{-1};

  QVBoxLayout *m_rootLayout;

  // Header
  QWidget *m_headerWidget;
  QHBoxLayout *m_headerLayout;
  QLabel *m_logoLabel;
  QLabel *m_titleLabel;
  QPushButton *m_toggleButton;

  // Navigation Items
  QScrollArea *m_scrollArea;
  QWidget *m_itemsContainer;
  QVBoxLayout *m_itemsLayout;

  // Footer
  QWidget *m_footerWidget{nullptr};

  struct SidebarItem {
    int id;
    QString text;
    QIcon icon;
    QPushButton *button{nullptr};
  };
  QVector<SidebarItem> m_items;

  QVariantAnimation *m_animation{nullptr};
  void updateItemVisuals();
};

} // namespace ui

#endif // UI_SIDEBAR_H

