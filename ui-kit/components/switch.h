#ifndef UI_SWITCH_H
#define UI_SWITCH_H

#include "../theme/theme.h"
#include <QAbstractButton>
#include <QVariantAnimation>

namespace ui {

class Switch : public QAbstractButton {
  Q_OBJECT
  Q_PROPERTY(qreal thumbOffset READ thumbOffset WRITE setThumbOffset)

public:
  explicit Switch(QWidget *parent = nullptr);
  explicit Switch(bool checked, QWidget *parent = nullptr);
  ~Switch() override = default;

  qreal thumbOffset() const;
  void setThumbOffset(qreal offset);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void checkStateSet() override;
  void nextCheckState() override;

private:
  qreal m_thumbOffset{0.0};
  QVariantAnimation *m_animation{nullptr};

  void updateThumbAnimation(bool checked);
};

} // namespace ui

#endif // UI_SWITCH_H
