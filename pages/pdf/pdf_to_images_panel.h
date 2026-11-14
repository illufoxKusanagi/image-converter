#ifndef PDF_TO_IMAGES_PANEL_H
#define PDF_TO_IMAGES_PANEL_H

#include "controllers/pdf/pdf_to_images_controller.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/select.h"
#include <QString>
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for configuring PDF-to-image export parameters.
 *
 * Provides controls to select the output image format, rendering resolution
 * (DPI), and an optional page range filter, and manages PDF-to-image export
 * execution through PdfToImagesController.
 */
class PdfToImagesPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the PDF to Images options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit PdfToImagesPanel(QWidget *parent = nullptr);
  ~PdfToImagesPanel() override = default;

  /**
   * @brief Returns the lowercase output image format key.
   * @return One of "png", "jpg", "webp", or "tiff" (defaults to "png").
   */
  QString selectedFormat() const;

  /**
   * @brief Returns the active image format selector index.
   * @return 0 for PNG, 1 for JPG, 2 for WEBP, 3 for TIFF.
   */
  int formatIndex() const;

  /**
   * @brief Returns the selected rendering resolution in DPI.
   * @return One of 72, 150, 300, or 600 (defaults to 300).
   */
  int selectedDpi() const;

  /**
   * @brief Returns the trimmed optional page filter specification.
   * @return User-entered page range string (e.g. "1-3, 5"), or empty for all
   * pages.
   */
  QString pageRangeSpec() const;

  /**
   * @brief Validates inputs, prompts for an output directory, and starts export.
   * @param sourcePaths List of source PDF file paths to export.
   * @param quality Output quality (0-100) for lossy formats; ignored for PNG.
   * @return True if export was started; false on validation failure or cancel.
   */
  bool execute(const QStringList &sourcePaths, int quality);

  /**
   * @brief Returns the underlying tool controller owned by this panel.
   * @return Pointer to the PdfToImagesController instance.
   */
  PdfToolController *controller() const { return m_controller; }

signals:
  /**
   * @brief Emitted when the user changes the output image format selection.
   * @param index Active format selector index.
   */
  void exportFormatChanged(int index);

  /**
   * @brief Emitted when the PDF-to-image export job finishes or is cancelled.
   */
  void executionFinished();

private slots:
  void onPdfToImagesFinished(const PdfToImagesController::Result &result);

private:
  ui::Select *m_formatSelect{nullptr};
  ui::FormField *m_formatField{nullptr};
  ui::Select *m_dpiSelect{nullptr};
  ui::FormField *m_dpiField{nullptr};
  ui::Input *m_pageRangeInput{nullptr};
  ui::FormField *m_pageRangeField{nullptr};
  PdfToImagesController *m_controller{nullptr};
};

#endif // PDF_TO_IMAGES_PANEL_H
