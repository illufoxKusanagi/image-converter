#ifndef SVG_OPTIONS_PANEL_H
#define SVG_OPTIONS_PANEL_H

#include "controllers/image/svg_convert_controller.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/switch.h"
#include "widgets/slider_widget.h"
#include <QLabel>
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for configuring SVG vector to raster/PDF conversion.
 *
 * Provides controls for output format, scale preset, custom pixel dimensions,
 * raster quality, aspect-ratio preservation, and opaque white background, and
 * owns the SvgConvertController executing batch conversions.
 */
class SvgOptionsPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the SVG conversion options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit SvgOptionsPanel(QWidget *parent = nullptr);
  ~SvgOptionsPanel() override = default;

  /**
   * @brief Synchronizes control enablement with active processing state.
   * @param isProcessing True while a conversion job is running, false when idle.
   */
  void syncProcessingState(bool isProcessing);

  /**
   * @brief Validates inputs, prompts for an output directory, and starts SVG conversion.
   * @param sourcePaths List of source SVG file paths to convert.
   * @return True if conversion was started; false on validation failure or cancel.
   */
  bool execute(const QStringList &sourcePaths);

  /**
   * @brief Returns the underlying SVG conversion controller owned by this panel.
   * @return Pointer to the SvgConvertController instance as PdfToolController.
   */
  PdfToolController *controller() const { return m_controller; }

public slots:
  /**
   * @brief Applies current theme colors and typography to panel labels.
   */
  void applyThemeStyles();

signals:
  /**
   * @brief Emitted when the SVG conversion job finishes or is cancelled.
   */
  void executionFinished();

private slots:
  void onFormatChanged(int index);
  void onScaleModeChanged(int index);
  void onSvgConvertFinished(const SvgConvertController::Result &result);
  void retranslateUi();

private:
  ui::Select *m_formatSelect{nullptr};
  ui::FormField *m_formatField{nullptr};
  ui::Select *m_scaleSelect{nullptr};
  ui::FormField *m_scaleField{nullptr};
  ui::Input *m_customWidthInput{nullptr};
  ui::FormField *m_customWidthField{nullptr};
  ui::Input *m_customHeightInput{nullptr};
  ui::FormField *m_customHeightField{nullptr};
  SliderWidget *m_qualitySlider{nullptr};
  ui::Switch *m_keepAspectSwitch{nullptr};
  QLabel *m_keepAspectLabel{nullptr};
  ui::Switch *m_whiteBgSwitch{nullptr};
  QLabel *m_whiteBgLabel{nullptr};
  SvgConvertController *m_controller{nullptr};
  bool m_isProcessing{false};
};

#endif // SVG_OPTIONS_PANEL_H
