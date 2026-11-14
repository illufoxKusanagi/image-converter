#include "pdf_quality_panel.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QStringList>

namespace {

struct QualityPreset {
  int index;
  int quality;
};

constexpr QualityPreset kQualityPresets[] = {
    {1, 20},
    {2, 50},
    {3, 80},
};

} // namespace

PdfQualityPanel::PdfQualityPanel(QWidget *parent) : QWidget(parent) {
  auto *attributeLayout = new QHBoxLayout(this);
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setValue(50);

  const QStringList presetOptions = {"Custom", "Web / Email", "Balanced",
                                     "High Quality (preserve images)"};
  m_presetSelect = new ui::Select(presetOptions, this);
  m_presetSelect->setCurrentIndex(2); // Default to Balanced (50)
  m_presetField =
      new ui::FormField("Compression Preset", m_presetSelect, this);
  m_presetField->setToolTip(
      "Text and vector graphics are retained. Below quality 80, images may "
      "lose detail. At quality 80 or above, images are not downsampled or "
      "newly JPEG-encoded. PDF rewriting can alter forms and annotations "
      "and invalidates digital signatures. Keep the original PDF.");

  connect(m_presetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfQualityPanel::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfQualityPanel::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_presetField, 1, Qt::AlignTop);
}

int PdfQualityPanel::quality() const {
  return m_qualitySlider ? m_qualitySlider->getValue() : 50;
}

void PdfQualityPanel::setPresetVisible(bool visible) {
  if (m_presetField) {
    m_presetField->setVisible(visible);
  }
}

void PdfQualityPanel::setControlsEnabled(bool enabled) {
  if (m_qualitySlider) {
    m_qualitySlider->setEnabled(enabled);
  }
  if (m_presetSelect) {
    m_presetSelect->setEnabled(enabled);
  }
}

void PdfQualityPanel::onPresetChanged(int index) {
  if (!m_qualitySlider) {
    return;
  }
  for (const auto &preset : kQualityPresets) {
    if (preset.index == index) {
      m_qualitySlider->blockSignals(true);
      m_qualitySlider->setValue(preset.quality);
      m_qualitySlider->blockSignals(false);
      return;
    }
  }
}

void PdfQualityPanel::onQualitySliderChanged() {
  if (!m_presetSelect || !m_qualitySlider) {
    return;
  }
  const int val = m_qualitySlider->getValue();
  int presetIndex = 0;
  for (const auto &preset : kQualityPresets) {
    if (preset.quality == val) {
      presetIndex = preset.index;
      break;
    }
  }
  m_presetSelect->blockSignals(true);
  m_presetSelect->setCurrentIndex(presetIndex);
  m_presetSelect->blockSignals(false);
}
