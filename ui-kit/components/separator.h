#ifndef UI_SEPARATOR_H
#define UI_SEPARATOR_H

#include "../theme/theme.h"
#include <QFrame>

namespace ui {

class Separator : public QFrame {
  Q_OBJECT

public:
  explicit Separator(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);
  ~Separator() override = default;

  Qt::Orientation orientation() const;
  void setOrientation(Qt::Orientation orientation);

private slots:
  void applyThemeStyles();

private:
  Qt::Orientation m_orientation{Qt::Horizontal};
};

} // namespace ui

#endif // UI_SEPARATOR_H

