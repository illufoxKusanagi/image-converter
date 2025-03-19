#include "slider_widget.h"

SliderWidget::SliderWidget(QWidget *parent, QString originTitle) {
  mainLayout = new QVBoxLayout(this);
  QString labelTitle = "Change your " + originTitle + " here";
  m_label = new QLabel(labelTitle, this);
  m_label->setStyleSheet("color: " + Colors::Grey900.name() + ";" +
                         TextStyle::BodyMediumRegular());
  m_slider = new QSlider(Qt::Horizontal, this);
  m_slider->setRange(0, 100);
  m_slider->setValue(60);

  m_spinBox = new QSpinBox(this);
  m_spinBox->setRange(0, 100);
  m_spinBox->setValue(60);
  connect(m_slider, &QSlider::valueChanged, m_spinBox, &QSpinBox::setValue);
  connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), m_slider,
          &QSlider::setValue);

  mainLayout->addWidget(m_label);
  mainLayout->addWidget(m_slider);
  mainLayout->addWidget(m_spinBox);
  setLayout(mainLayout);
}