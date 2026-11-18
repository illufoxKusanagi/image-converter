#ifndef UI_SCROLL_AREA_H
#define UI_SCROLL_AREA_H

#include <QScrollArea>
#include <QString>

class QAbstractScrollArea;

namespace ui {

class ScrollArea : public QScrollArea {
  Q_OBJECT

public:
  explicit ScrollArea(QWidget *parent = nullptr);

  static QString scrollBarStyleSheet();
  static void applyScrollBarStyles(QAbstractScrollArea *area);

private slots:
  void applyThemeStyles();
};

} // namespace ui

#endif // UI_SCROLL_AREA_H
