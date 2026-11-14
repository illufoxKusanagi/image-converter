#ifndef PDF_QUALITY_PANEL_H
#define PDF_QUALITY_PANEL_H

#include "controllers/pdf/compress_controller.h"
#include "controllers/pdf/grayscale_controller.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "widgets/slider_widget.h"
#include <QStringList>
#include <QWidget>

/**
 * @brief Quality and compression preset panel for PDF operations.
 *
 * Pairs a PDF quality slider with a synchronized compression preset selector
 * so users can pick predefined quality levels or enter a custom value. Also
 * owns and orchestrates the Compress and Grayscale PDF controllers.
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

  /**
   * @brief Prompts for output location and starts PDF compression.
   * @param sourcePaths Selected input PDF file paths.
   * @return True if the compression job started, false on validation or cancel.
   */
  bool executeCompress(const QStringList &sourcePaths);

  /**
   * @brief Prompts for output location and starts PDF grayscale conversion.
   * @param sourcePaths Selected input PDF file paths.
   * @return True if the grayscale job started, false on validation or cancel.
   */
  bool executeGrayscale(const QStringList &sourcePaths);

  /**
   * @brief Returns the owned Compress PDF controller.
   * @return Pointer to the internal CompressController as PdfToolController.
   */
  PdfToolController *compressController() const { return m_compressController; }

  /**
   * @brief Returns the owned Grayscale PDF controller.
   * @return Pointer to the internal GrayscaleController as PdfToolController.
   */
  PdfToolController *grayscaleController() const {
    return m_grayscaleController;
  }

signals:
  /**
   * @brief Emitted when a Compress or Grayscale execution finishes.
   */
  void executionFinished();

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

  /**
   * @brief Handles completion of a Compress PDF job and presents results.
   * @param result Outcome summary emitted by CompressController.
   */
  void onCompressFinished(const CompressController::Result &result);

  /**
   * @brief Handles completion of a Grayscale PDF job and presents results.
   * @param result Outcome summary emitted by GrayscaleController.
   */
  void onGrayscaleFinished(const GrayscaleController::Result &result);

private:
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_presetSelect{nullptr};
  ui::FormField *m_presetField{nullptr};
  CompressController *m_compressController{nullptr};
  GrayscaleController *m_grayscaleController{nullptr};
  int m_lastBatchCount{0};
};

#endif // PDF_QUALITY_PANEL_H
