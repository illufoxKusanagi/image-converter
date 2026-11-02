#ifndef UI_MENU_H
#define UI_MENU_H

#include <QMenu>

namespace ui {

class Menu : public QMenu {
  Q_OBJECT

public:
  explicit Menu(QWidget *parent = nullptr);
  explicit Menu(const QString &title, QWidget *parent = nullptr);
  ~Menu() override = default;

private:
  void applyThemeStyles();
};

using DropdownMenu = Menu;
using ContextMenu = Menu;

} // namespace ui

#endif // UI_MENU_H
