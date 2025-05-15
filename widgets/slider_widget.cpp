#include "slider_widget.h"

SliderWidget::SliderWidget(QWidget *parent, QString originTitle)
    : QWidget(parent), m_value(60) {
  mainLayout = new QVBoxLayout(this);
  QString labelTitle = originTitle;
  m_label = new QLabel(labelTitle, this);
  setStyleSheet("QLabel {"
                "color: " +
                Colors::Grey900.name() + ";" + TextStyle::BodyMediumRegular() +
                "}"
                "QSpinBox {"
                "    color: " +
                Colors::Primary600.name() +
                ";"
                "    background-color: " +
                Colors::StandardWhite.name() +
                ";"
                "    border: 1px solid " +
                Colors::Primary500.name() +
                ";"
                "    border-radius: 8px;"
                "    padding: 2px 5px;"
                "    min-height: 28px;"
                "    font-size: 14px;"
                "}"
                "QSpinBox::up-button {"
                "    subcontrol-origin: border;"
                "    subcontrol-position: top right;"
                "    width: 16px;"
                "    border-left: 1px solid " +
                Colors::Primary500.name() +
                ";"
                "    border-bottom: 1px solid " +
                Colors::Primary500.name() +
                ";"
                "    border-top-right-radius: 8px;"
                "    background-color: " +
                Colors::Primary100.name() +
                ";"
                "}"
                "QSpinBox::up-button:hover {"
                "    background-color: " +
                Colors::Primary200.name() +
                ";"
                "}"
                "QSpinBox::up-button:pressed {"
                "    background-color: " +
                Colors::Primary300.name() +
                ";"
                "}"
                "QSpinBox::up-arrow {"
                "    image: url(:/icons/icons/arrow-up-black.svg);"

                "    width: 10px;"
                "    height: 10px;"
                "}"
                "QSpinBox::down-button {"
                "    subcontrol-origin: border;"
                "    subcontrol-position: bottom right;"
                "    width: 16px;"
                "    border-left: 1px solid " +
                Colors::Primary500.name() +
                ";"
                "    border-top: 1px solid " +
                Colors::Primary500.name() +
                ";"
                "    border-bottom-right-radius: 8px;"
                "    background-color: " +
                Colors::Primary100.name() +
                ";"
                "}"
                "QSpinBox::down-button:hover {"
                "    background-color: " +
                Colors::Primary200.name() +
                ";"
                "}"
                "QSpinBox::down-button:pressed {"
                "    background-color: " +
                Colors::Primary300.name() +
                ";"
                "}"
                "QSpinBox::down-arrow {"
                "    image: url(:/icons/icons/arrow-down-black.svg);"
                "    width: 10px;"
                "    height: 10px;"
                "}"
                "QSpinBox:disabled {"
                "    background-color: " +
                Colors::Grey100.name() +
                ";"
                "    color: " +
                Colors::Grey400.name() +
                ";"
                "    border-color: " +
                Colors::Grey300.name() +
                ";"
                "}");
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
