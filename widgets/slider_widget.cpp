#include "slider_widget.h"

SliderWidget::SliderWidget(QWidget *parent, QString originTitle)
    : QWidget(parent), m_value(100) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(8);
  QString labelTitle = originTitle;
  m_label = new QLabel(labelTitle, this);
  setStyleSheet(s_labelStyling + s_sliderStyling + s_spinBoxStyling);
  m_slider = new QSlider(Qt::Horizontal, this);
  m_slider->setRange(0, 100);
  m_slider->setValue(m_value);
  m_spinBox = new QSpinBox(this);
  m_spinBox->setRange(0, 100);
  m_spinBox->setValue(m_value);
  m_spinBox->setFixedHeight(40);
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

const QString SliderWidget::s_labelStyling =
    "QLabel {"
    "color: " +
    Colors::Grey900.name() + ";" + TextStyle::BodyMediumRegular() + "}";

const QString SliderWidget::s_sliderStyling =
    "QSlider::groove:horizontal {"
    "    border: none;"
    "    height: 6px;"
    "    background-color: " +
    Colors::Grey200.name() +
    ";"
    "    border-radius: 3px;"
    "}"

    "QSlider::handle:horizontal {"
    "    background-color: " +
    Colors::Primary500.name() +
    ";"
    "    border: none;"
    "    width: 16px;"
    "    margin: -5px 0px;"
    "    height: 16px;"
    "    border-radius: 8px;"
    "}"

    "QSlider::handle:horizontal:hover {"
    "    background-color: " +
    Colors::Primary600.name() +
    ";"
    "}"

    "QSlider::handle:horizontal:pressed {"
    "    background-color: " +
    Colors::Primary700.name() +
    ";"
    "}"

    "QSlider::sub-page:horizontal {"
    "    background-color: " +
    Colors::Primary300.name() +
    ";"
    "    border-radius: 3px;"
    "}"

    "QSlider:disabled {"
    "    opacity: 0.5;"
    "}";

const QString SliderWidget::s_spinBoxStyling =
    "QSpinBox {"
    "    color: " +
    Colors::Primary600.name() +
    ";"
    "    background-color: " +
    Colors::StandardWhite.name() + ";" + TextStyle::BodySmallRegular() +
    "    border: 1px solid " + Colors::Primary500.name() +
    ";"
    "    border-radius: 8px;"
    "    padding: 2px 16px 2px 4px;"
    "}"
    "QSpinBox::up-button {"
    "    subcontrol-origin: border;"
    "    subcontrol-position: top right;"
    "    width: 32px;"
    "    border-left: 1px solid " +
    Colors::Primary500.name() +
    ";"
    "    border-top: 1px solid " +
    Colors::Primary500.name() +
    ";"
    "    border-bottom: 1px solid " +
    Colors::Primary500.name() +
    ";"
    "    border-right: 1px solid " +
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
    "    width: 32px;"
    "    border-left: 1px solid " +
    Colors::Primary500.name() +
    ";"
    "    border-top: 1px solid " +
    Colors::Primary500.name() +
    ";"
    "    border-bottom: 1px solid " +
    Colors::Primary500.name() +
    ";"
    "    border-right: 1px solid " +
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
    "}";