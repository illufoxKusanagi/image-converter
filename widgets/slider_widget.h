#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include "ui-kit/theme/theme.h"
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

class SliderWidget : public QWidget {
  Q_OBJECT
public:
  explicit SliderWidget(QWidget *parent = nullptr, QString originTitle = "");
  int getValue() const;
  int value() const { return getValue(); }
  void setValue(int value);
  void setTitle(const QString &title);
  void setEnabled(bool enabled);

signals:
  void valueChanged();

private slots:
  void updateSliderValue();
  void updateSpinBoxValue();
  void applyThemeStyles();

private:
  QVBoxLayout *mainLayout;
  QLabel *m_label;
  QSlider *m_slider;
  QSpinBox *m_spinBox;
  int m_value;
};

#endif // SLIDERWIDGET_H
