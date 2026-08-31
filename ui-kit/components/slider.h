#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include "../theme/theme.h"
#include <QSlider>

namespace ui {

class Slider : public QSlider {
  Q_OBJECT

public:
  explicit Slider(QWidget *parent = nullptr);
  explicit Slider(Qt::Orientation orientation, QWidget *parent = nullptr);
  ~Slider() override = default;

private slots:
  void applyThemeStyles();
};

} // namespace ui

#endif // UI_SLIDER_H
