#ifndef UI_SCROLL_AREA_H
#define UI_SCROLL_AREA_H

#include <QScrollArea>

namespace ui {

class ScrollArea : public QScrollArea {
  Q_OBJECT

public:
  explicit ScrollArea(QWidget *parent = nullptr);

private slots:
  void applyThemeStyles();
};

} // namespace ui

#endif // UI_SCROLL_AREA_H
