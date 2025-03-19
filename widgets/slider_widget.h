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

signals:
  void valueChanged();

private:
  QVBoxLayout *mainLayout;
  QLabel *m_label;
  QSlider *m_slider;
  QSpinBox *m_spinBox;
};

#endif // INPUTWIDGET_H
