#include "slider_widget.h"
#include "ui-kit/theme/style_helper.h"

SliderWidget::SliderWidget(QWidget *parent, QString originTitle)
    : QWidget(parent), m_value(100) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(8);

  m_label = new QLabel(originTitle, this);

  QHBoxLayout *controlsLayout = new QHBoxLayout();
  controlsLayout->setContentsMargins(0, 0, 0, 0);
  controlsLayout->setSpacing(12);

  m_slider = new QSlider(Qt::Horizontal, this);
  m_slider->setRange(0, 100);
  m_slider->setValue(m_value);

  m_spinBox = new QSpinBox(this);
  m_spinBox->setRange(0, 100);
  m_spinBox->setValue(m_value);
  m_spinBox->setFixedWidth(68);
  m_spinBox->setFixedHeight(40);
  m_spinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  connect(m_slider, &QSlider::valueChanged, this,
          &SliderWidget::updateSpinBoxValue);
  connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &SliderWidget::updateSliderValue);

  controlsLayout->addWidget(m_slider, 1, Qt::AlignVCenter);
  controlsLayout->addWidget(m_spinBox, 0, Qt::AlignVCenter);

  mainLayout->addWidget(m_label);
  mainLayout->addLayout(controlsLayout);
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this, &SliderWidget::applyThemeStyles);
  applyThemeStyles();
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

int SliderWidget::getValue() const { return m_value; }

void SliderWidget::setValue(int value) {
  m_value = qBound(0, value, 100);
  if (m_slider) {
    m_slider->blockSignals(true);
    m_slider->setValue(m_value);
    m_slider->blockSignals(false);
  }
  if (m_spinBox) {
    m_spinBox->blockSignals(true);
    m_spinBox->setValue(m_value);
    m_spinBox->blockSignals(false);
  }
  emit valueChanged();
}

void SliderWidget::setTitle(const QString &title) {
  if (m_label) {
    m_label->setText(title);
  }
}

void SliderWidget::setEnabled(bool enabled) {
  QWidget::setEnabled(enabled);
  if (m_slider) m_slider->setEnabled(enabled);
  if (m_spinBox) m_spinBox->setEnabled(enabled);
}

void SliderWidget::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  m_label->setFont(t.font(t.sizeSm, QFont::DemiBold));
  m_label->setStyleSheet(QString("color: %1; background: transparent;")
                             .arg(ui::StyleHelper::toHexString(c.foreground)));

  m_spinBox->setFont(t.font(t.sizeSm, QFont::Normal));
  m_spinBox->setStyleSheet(QString(
      "QSpinBox {"
      "  background-color: %1;"
      "  color: %2;"
      "  border: 1px solid %3;"
      "  border-radius: %4px;"
      "  padding: 0px 22px 0px 10px;"
      "  min-height: 38px;"
      "}"
      "QSpinBox:focus {"
      "  border: 2px solid %5;"
      "}"
      "QSpinBox:disabled {"
      "  color: %8;"
      "  background-color: %9;"
      "  border: 1px solid %3;"
      "}"
      "QSpinBox::up-button {"
      "  subcontrol-origin: border;"
      "  subcontrol-position: top right;"
      "  width: 20px;"
      "  height: 19px;"
      "  background-color: %6;"
      "  border-left: 1px solid %3;"
      "  border-bottom: 1px solid %3;"
      "  border-top-right-radius: %4px;"
      "}"
      "QSpinBox::up-button:hover {"
      "  background-color: %7;"
      "}"
      "QSpinBox::up-arrow {"
      "  image: url(:/icons/icons/lucide-chevron-up.svg);"
      "  width: 10px;"
      "  height: 10px;"
      "}"
      "QSpinBox::down-button {"
      "  subcontrol-origin: border;"
      "  subcontrol-position: bottom right;"
      "  width: 20px;"
      "  height: 19px;"
      "  background-color: %6;"
      "  border-left: 1px solid %3;"
      "  border-bottom-right-radius: %4px;"
      "}"
      "QSpinBox::down-button:hover {"
      "  background-color: %7;"
      "}"
      "QSpinBox::down-arrow {"
      "  image: url(:/icons/icons/lucide-chevron-down.svg);"
      "  width: 10px;"
      "  height: 10px;"
      "}")
      .arg(ui::StyleHelper::toHexString(c.card))
      .arg(ui::StyleHelper::toHexString(c.foreground))
      .arg(ui::StyleHelper::toHexString(c.border))
      .arg(r.md)
      .arg(ui::StyleHelper::toHexString(c.primary))
      .arg(ui::StyleHelper::toHexString(c.secondary))
      .arg(ui::StyleHelper::toHexString(c.accent))
      .arg(ui::StyleHelper::toHexString(c.mutedForeground))
      .arg(ui::StyleHelper::toHexString(c.muted)));

  m_slider->setStyleSheet(QString(
      "QSlider::groove:horizontal {"
      "  border: none;"
      "  height: 6px;"
      "  background-color: %1;"
      "  border-radius: 3px;"
      "}"
      "QSlider::sub-page:horizontal {"
      "  background-color: %2;"
      "  border-radius: 3px;"
      "}"
      "QSlider::handle:horizontal {"
      "  background-color: %3;"
      "  border: 2px solid %4;"
      "  width: 16px;"
      "  margin: -6px 0px;"
      "  height: 16px;"
      "  border-radius: 9px;"
      "}"
      "QSlider::handle:horizontal:hover {"
      "  background-color: %5;"
      "}"
      "QSlider::sub-page:horizontal:disabled {"
      "  background-color: %6;"
      "}"
      "QSlider::handle:horizontal:disabled {"
      "  border: 2px solid %6;"
      "  background-color: %3;"
      "}")
      .arg(ui::StyleHelper::toHexString(c.secondary))
      .arg(ui::StyleHelper::toHexString(c.primary))
      .arg(ui::StyleHelper::toHexString(c.background))
      .arg(ui::StyleHelper::toHexString(c.primary))
      .arg(ui::StyleHelper::toHexString(c.primary))
      .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
}
