#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief A QWidget combining a slider and spin box with synchronized values.
 *
 * The SliderWidget displays a label, a slider, and a spin box in a vertical layout. Changing the value in either the slider or spin box updates the other, and the widget emits a signal when the value changes.
 *
 * @param parent Optional parent widget.
 * @param originTitle Optional label text displayed above the controls.
 *
 * @signal valueChanged() Emitted whenever the value is changed by the user.
 */
class SliderWidget : public QWidget {
  Q_OBJECT
public:
  explicit SliderWidget(QWidget *parent = nullptr, QString originTitle = "");
  int getValue();

signals:
  void valueChanged();

private slots:
  void updateSliderValue();
  void updateSpinBoxValue();

private:
  QVBoxLayout *mainLayout;
  QLabel *m_label;
  QSlider *m_slider;
  QSpinBox *m_spinBox;
  int m_value;
};

#endif // INPUTWIDGET_H
