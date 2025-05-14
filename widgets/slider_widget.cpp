#include "slider_widget.h"

/**
 * @brief Constructs a slider widget with a label, synchronized slider, and spin box for value selection.
 *
 * Creates a vertical layout containing a styled label, a horizontal slider, and a spin box, all representing a value between 0 and 100. The slider and spin box are initialized to 60 and kept in sync, allowing users to adjust the value using either control.
 *
 * @param parent The parent widget.
 * @param originTitle The text to display in the label above the slider.
 */
SliderWidget::SliderWidget(QWidget *parent, QString originTitle)
    : QWidget(parent), m_value(60) {
  mainLayout = new QVBoxLayout(this);
  QString labelTitle = originTitle;
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

/**
 * @brief Updates the slider to match the spin box value and emits a valueChanged signal.
 *
 * Synchronizes the slider's value with the spin box, updates the internal value, and emits the valueChanged signal to notify listeners of the change.
 */
void SliderWidget::updateSliderValue() {
  m_slider->blockSignals(true);
  m_slider->setValue(m_spinBox->value());
  m_slider->blockSignals(false);
  m_value = m_spinBox->value();
  emit valueChanged();
}

/**
 * @brief Updates the spin box to match the slider's value and emits a valueChanged signal.
 *
 * Synchronizes the spin box with the current value of the slider, updates the internal value, and notifies listeners of the change.
 */
void SliderWidget::updateSpinBoxValue() {
  m_spinBox->blockSignals(true);
  m_spinBox->setValue(m_slider->value());
  m_spinBox->blockSignals(false);
  m_value = m_slider->value();
  emit valueChanged();
}

/**
 * @brief Returns the current value selected by the slider and spin box.
 *
 * @return int The current value, ranging from 0 to 100.
 */
int SliderWidget::getValue() { return m_value; }