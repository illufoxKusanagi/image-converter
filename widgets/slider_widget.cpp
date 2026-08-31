#include "slider_widget.h"
#include "ui-kit/theme/style_helper.h"

SliderWidget::SliderWidget(QWidget *parent, QString originTitle)
    : QWidget(parent), m_value(100) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(ui::Theme::instance().spacing().space2);

  m_label = new QLabel(originTitle, this);

  m_spinBox = new QSpinBox(this);
  m_spinBox->setRange(0, 100);
  m_spinBox->setValue(m_value);
  m_spinBox->setFixedHeight(38);
  m_spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_spinBox->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  m_slider = new QSlider(Qt::Horizontal, this);
  m_slider->setRange(0, 100);
  m_slider->setValue(m_value);
  m_slider->setFixedHeight(22);

  connect(m_slider, &QSlider::valueChanged, this,
          &SliderWidget::updateSpinBoxValue);
  connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &SliderWidget::updateSliderValue);

  mainLayout->addWidget(m_label);
  mainLayout->addWidget(m_spinBox);
  mainLayout->addWidget(m_slider);
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &SliderWidget::applyThemeStyles);
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
  if (m_slider)
    m_slider->setEnabled(enabled);
  if (m_spinBox)
    m_spinBox->setEnabled(enabled);
}

void SliderWidget::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();
  bool isDark = ui::Theme::instance().isDark();

  m_label->setStyleSheet(
      QString("color: %1; background: transparent; font-size: %2px; "
              "font-weight: 600; font-family: '%3';")
          .arg(ui::StyleHelper::toHexString(c.foreground))
          .arg(t.sizeBase)
          .arg(t.fontFamily));

  QString cardHex = ui::StyleHelper::toHexString(c.card);
  QString fgHex = ui::StyleHelper::toHexString(c.foreground);
  QString borderHex = ui::StyleHelper::toHexString(c.primary);
  QString hoverBorderHex = ui::StyleHelper::toHexString(
      ui::StyleHelper::hoverColor(c.primary, isDark));
  QString mutedFgHex = ui::StyleHelper::toHexString(c.mutedForeground);
  QString mutedHex = ui::StyleHelper::toHexString(c.muted);
  QString secondaryHex = ui::StyleHelper::toHexString(c.secondary);
  QString bgHex = ui::StyleHelper::toHexString(c.background);
  QString compartmentBgHex = isDark ? "#27272a" : "#ede9fe";
  QString compartmentHoverHex = isDark ? "#3f3f46" : "#ddd6fe";
  QString compartmentPressedHex = isDark ? "#52525b" : "#c4b5fd";
  QString upIcon = isDark ? ":/icons/icons/arrow-up.svg"
                          : ":/icons/icons/arrow-up-black.svg";
  QString downIcon = isDark ? ":/icons/icons/arrow-down.svg"
                            : ":/icons/icons/arrow-down-black.svg";

  m_spinBox->setFont(t.font(t.sizeBase, QFont::Normal));
  m_spinBox->setStyleSheet(
      "QSpinBox {"
      "  background-color: " +
      cardHex +
      ";"
      "  color: " +
      fgHex +
      ";"
      "  border: 1px solid " +
      borderHex +
      ";"
      "  border-radius: " +
      QString::number(r.md) +
      "px;"
      "  padding: 0px 36px 0px 12px;"
      "  font-size: " +
      QString::number(t.sizeBase) +
      "px;"
      "}"
      "QSpinBox:hover, QSpinBox:focus {"
      "  border-color: " +
      hoverBorderHex +
      ";"
      "}"
      "QSpinBox:disabled {"
      "  color: " +
      mutedFgHex +
      ";"
      "  background-color: " +
      mutedHex +
      ";"
      "  border-color: " +
      borderHex +
      ";"
      "}"
      "QSpinBox::up-button {"
      "  subcontrol-origin: border;"
      "  subcontrol-position: top right;"
      "  width: 32px;"
      "  border: 1px solid " +
      borderHex +
      ";"
      "  border-top-right-radius: " +
      QString::number(r.md - 1) +
      "px;"
      "  background-color: " +
      compartmentBgHex +
      ";"
      "}"
      "QSpinBox:hover::up-button, QSpinBox:focus::up-button {"
      "  border-color: " +
      hoverBorderHex +
      ";"
      "}"
      "QSpinBox::up-button:hover {"
      "  background-color: " +
      compartmentHoverHex +
      ";"
      "}"
      "QSpinBox::up-button:pressed {"
      "  background-color: " +
      compartmentPressedHex +
      ";"
      "}"
      "QSpinBox::up-arrow {"
      "  image: url(" +
      upIcon +
      ");"
      "  width: 10px;"
      "  height: 10px;"
      "}"
      "QSpinBox::down-button {"
      "  subcontrol-origin: border;"
      "  subcontrol-position: bottom right;"
      "  width: 32px;"
      "  border: 1px solid " +
      borderHex +
      ";"
      "  border-bottom-right-radius: " +
      QString::number(r.md - 1) +
      "px;"
      "  background-color: " +
      compartmentBgHex +
      ";"
      "}"
      "QSpinBox:hover::down-button, QSpinBox:focus::down-button {"
      "  border-color: " +
      hoverBorderHex +
      ";"
      "}"
      "QSpinBox::down-button:hover {"
      "  background-color: " +
      compartmentHoverHex +
      ";"
      "}"
      "QSpinBox::down-button:pressed {"
      "  background-color: " +
      compartmentPressedHex +
      ";"
      "}"
      "QSpinBox::down-arrow {"
      "  image: url(" +
      downIcon +
      ");"
      "  width: 10px;"
      "  height: 10px;"
      "}");

  m_slider->setStyleSheet("QSlider::groove:horizontal {"
                          "  border: none;"
                          "  height: 6px;"
                          "  background-color: " +
                          secondaryHex +
                          ";"
                          "  border-radius: 3px;"
                          "}"
                          "QSlider::sub-page:horizontal {"
                          "  background-color: " +
                          borderHex +
                          ";"
                          "  border-radius: 3px;"
                          "}"
                          "QSlider::handle:horizontal {"
                          "  background-color: " +
                          bgHex +
                          ";"
                          "  border: 2px solid " +
                          borderHex +
                          ";"
                          "  width: 16px;"
                          "  margin: -6px 0px;"
                          "  height: 16px;"
                          "  border-radius: 9px;"
                          "}"
                          "QSlider::handle:horizontal:hover {"
                          "  background-color: " +
                          borderHex +
                          ";"
                          "}"
                          "QSlider::sub-page:horizontal:disabled {"
                          "  background-color: " +
                          mutedFgHex +
                          ";"
                          "}"
                          "QSlider::handle:horizontal:disabled {"
                          "  border: 2px solid " +
                          mutedFgHex +
                          ";"
                          "  background-color: " +
                          bgHex +
                          ";"
                          "}");
}
