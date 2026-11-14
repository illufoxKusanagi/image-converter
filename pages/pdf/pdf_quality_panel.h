#ifndef PDF_QUALITY_PANEL_H
#define PDF_QUALITY_PANEL_H

#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "widgets/slider_widget.h"
#include <QWidget>

/**
 * @brief Quality and compression preset panel for PDF operations.
 *
 * Pairs a PDF quality slider with a synchronized compression preset selector
 * so users can pick predefined quality levels or enter a custom value.
 */
class PdfQualityPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the PDF quality and preset panel.
   * @param parent Optional Qt parent widget.
   */
  explicit PdfQualityPanel(QWidget *parent = nullptr);
  ~PdfQualityPanel() override = default;

  /**
   * @brief Returns the currently selected PDF quality value.
   * @return Quality value in the slider's configured range (defaults to 50).
   */
  int quality() const;

  /**
   * @brief Shows or hides the compression preset field.
   * @param visible True to show the preset selector, false to hide it.
   */
  void setPresetVisible(bool visible);

  /**
   * @brief Enables or disables the quality slider and preset selector.
   * @param enabled True to allow user interaction, false to disable controls.
   */
  void setControlsEnabled(bool enabled);

private slots:
  /**
   * @brief Updates the quality slider to match the chosen preset index.
   * @param index Active index from the compression preset combo box.
   */
  void onPresetChanged(int index);

  /**
   * @brief Synchronizes the compression preset selector with slider changes.
   */
  void onQualitySliderChanged();

private:
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_presetSelect{nullptr};
  ui::FormField *m_presetField{nullptr};
};

#endif // PDF_QUALITY_PANEL_H
