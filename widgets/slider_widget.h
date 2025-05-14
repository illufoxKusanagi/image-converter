#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

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
