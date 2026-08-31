#ifndef UI_SPINNER_H
#define UI_SPINNER_H

#include "../theme/theme.h"
#include <QVariantAnimation>
#include <QWidget>

namespace ui {

class Spinner : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int angle READ angle WRITE setAngle)

public:
  explicit Spinner(QWidget *parent = nullptr);
  explicit Spinner(int size, QWidget *parent = nullptr);
  ~Spinner() override = default;

  int size() const;
  void setSpinnerSize(int size);

  QColor color() const;
  void setColor(const QColor &color);

  int angle() const;
  void setAngle(int angle);

  void start();
  void stop();
  bool isSpinning() const;

protected:
  void paintEvent(QPaintEvent *event) override;
  QSize sizeHint() const override;

private:
  int m_size{24};
  int m_angle{0};
  QColor m_customColor;
  QVariantAnimation *m_animation{nullptr};
};

} // namespace ui

#endif // UI_SPINNER_H
