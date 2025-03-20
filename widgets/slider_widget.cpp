#include "slider_widget.h"

SliderWidget::SliderWidget(QWidget *parent, QString originTitle)
    : QWidget(parent), m_value(60) {
  mainLayout = new QVBoxLayout(this);
  QString labelTitle = "Change your " + originTitle + " here";
  m_label = new QLabel(labelTitle, this);
  m_label->setStyleSheet("color: " + Colors::Grey900.name() + ";" +
                         TextStyle::BodyMediumRegular());
  m_slider = new QSlider(Qt::Horizontal, this);
  m_slider->setRange(0, 100);
  m_slider->setValue(m_value);

  m_spinBox = new QSpinBox(this);
  m_spinBox->setRange(0, 100);
  m_spinBox->setValue(m_value);
  connect(m_slider, &QSlider::valueChanged, this,
          &SliderWidget::updateSpinBoxValue);
  connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &SliderWidget::updateSliderValue);
  // connect(m_slider, &QSlider::valueChanged, m_spinBox, &QSpinBox::setValue);
  // connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_slider,
  //         &QSlider::setValue);
  mainLayout->addWidget(m_label);
  mainLayout->addWidget(m_slider);
  mainLayout->addWidget(m_spinBox);
  setLayout(mainLayout);
}

void SliderWidget::updateSliderValue() {
  m_slider->blockSignals(true);
  m_slider->setValue(m_spinBox->value());
  m_slider->blockSignals(false);
  m_value = m_spinBox->value();
  emit valueChanged();
}

void SliderWidget::updateSpinBoxValue() {
  m_spinBox->blockSignals(true);
  m_spinBox->setValue(m_slider->value());
  m_spinBox->blockSignals(false);
  m_value = m_slider->value();
  emit valueChanged();
}

int SliderWidget::getValue() { return m_value; }