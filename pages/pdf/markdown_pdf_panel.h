#ifndef MARKDOWN_PDF_PANEL_H
#define MARKDOWN_PDF_PANEL_H

#include "controllers/pdf/markdown_pdf_controller.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for configuring Markdown/HTML to PDF conversion.
 *
 * Provides controls to select page size, orientation, page margins, and base
 * font size for rendered documents.
 */
class MarkdownPdfPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Markdown/HTML to PDF options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit MarkdownPdfPanel(QWidget *parent = nullptr);
  ~MarkdownPdfPanel() override = default;

  /**
   * @brief Returns whether US Letter page size is selected.
   * @return True if US Letter is selected, false for A4.
   */
  bool useLetterPageSize() const;

  /**
   * @brief Returns whether landscape orientation is selected.
   * @return True if Landscape is selected, false for Portrait.
   */
  bool landscape() const;

  /**
   * @brief Returns the selected page margin in points.
   * @return One of 18, 36, 54, or 72 (defaults to 36).
   */
  int marginPt() const;

  /**
   * @brief Returns the selected base font size in points.
   * @return One of 9, 10, 11, 12, 14, or 16 (defaults to 10).
   */
  int baseFontPt() const;

  /**
   * @brief Prompts for an output directory and starts Markdown/HTML to PDF conversion.
   * @param sourcePaths Input Markdown (.md) or HTML (.html) file paths.
   * @return True if conversion was started, false if validation or prompt cancelled.
   */
  bool execute(const QStringList &sourcePaths);

  /**
   * @brief Returns the underlying Markdown/HTML to PDF controller.
   * @return Pointer to the owned MarkdownPdfController as PdfToolController.
   */
  PdfToolController *controller() const { return m_controller; }

signals:
  /**
   * @brief Emitted when the Markdown/HTML to PDF job finishes.
   */
  void executionFinished();

private slots:
  /**
   * @brief Handles completion of the Markdown/HTML to PDF job and shows feedback.
   * @param result Execution result emitted by the controller.
   */
  void onMarkdownPdfFinished(const MarkdownPdfController::Result &result);

private:
  MarkdownPdfController *m_controller{nullptr};
  ui::Select *m_pageSizeSelect{nullptr};
  ui::FormField *m_pageSizeField{nullptr};
  ui::Select *m_orientationSelect{nullptr};
  ui::FormField *m_orientationField{nullptr};
  ui::Select *m_marginSelect{nullptr};
  ui::FormField *m_marginField{nullptr};
  ui::Select *m_fontSizeSelect{nullptr};
  ui::FormField *m_fontSizeField{nullptr};
};

#endif // MARKDOWN_PDF_PANEL_H
