#ifndef PDF_TO_IMAGES_PANEL_H
#define PDF_TO_IMAGES_PANEL_H

#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/select.h"
#include <QString>
#include <QWidget>

/**
 * @brief Options panel for configuring PDF-to-image export parameters.
 *
 * Provides controls to select the output image format, rendering resolution
 * (DPI), and an optional page range filter.
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

signals:
  /**
   * @brief Emitted when the user changes the output image format selection.
   * @param index Active format selector index.
   */
  void exportFormatChanged(int index);

private:
  ui::Select *m_formatSelect{nullptr};
  ui::FormField *m_formatField{nullptr};
  ui::Select *m_dpiSelect{nullptr};
  ui::FormField *m_dpiField{nullptr};
  ui::Input *m_pageRangeInput{nullptr};
  ui::FormField *m_pageRangeField{nullptr};
};

#endif // PDF_TO_IMAGES_PANEL_H
