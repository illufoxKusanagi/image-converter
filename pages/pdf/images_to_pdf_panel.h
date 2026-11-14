#ifndef IMAGES_TO_PDF_PANEL_H
#define IMAGES_TO_PDF_PANEL_H

#include "controllers/pdf/images_to_pdf_controller.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include <QLabel>
#include <QListWidget>
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for configuring Images-to-PDF conversion.
 *
 * Displays an interactive reorderable list of selected image files synced from
 * the drop zone along with page size, orientation, and margin selectors.
 */
class ImagesToPdfPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Images-to-PDF options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit ImagesToPdfPanel(QWidget *parent = nullptr);
  ~ImagesToPdfPanel() override = default;

  /**
   * @brief Synchronizes the image list with the current drop-zone file paths.
   *
   * Preserves existing item ordering for paths still present in @p dropPaths,
   * skips PDF files, deduplicates paths, and appends newly added images.
   *
   * @param dropPaths Current list of file paths from the drop widget.
   */
  void syncFromPaths(const QStringList &dropPaths);

  /**
   * @brief Returns the image file paths in their current visual page order.
   * @return Ordered list of full file paths stored in each item's Qt::UserRole.
   */
  QStringList orderedImagePaths() const;

  /**
   * @brief Returns the selected page size option index.
   */
  int pageSizeMode() const;

  /**
   * @brief Returns the selected orientation option index.
   */
  int orientationMode() const;

  /**
   * @brief Returns the selected page margin option index.
   */
  int marginMode() const;

  /**
   * @brief Syncs drop-zone images, prompts for an output path, and starts PDF
   * creation.
   * @param dropPaths Current list of file paths from the drop widget.
   * @return @c true if background PDF creation started, @c false if aborted or
   * invalid.
   */
  bool execute(const QStringList &dropPaths);

  /**
   * @brief Returns the controller managing background Images-to-PDF jobs.
   */
  PdfToolController *controller() const { return m_controller; }

public slots:
  /**
   * @brief Applies current theme colors, radii, and typography to child widgets.
   */
  void applyThemeStyles();

  /**
   * @brief Moves the currently selected image one position up in page order.
   */
  void moveItemUp();

  /**
   * @brief Moves the currently selected image one position down in page order.
   */
  void moveItemDown();

signals:
  /**
   * @brief Emitted when the user clicks "Sync from Drop Zone".
   */
  void syncRequested();

  /**
   * @brief Emitted when the background Images-to-PDF job finishes or is
   * cancelled.
   */
  void executionFinished();

private slots:
  /**
   * @brief Handles completion of the background Images-to-PDF job and notifies
   * the user.
   * @param result Execution summary, output path/size, or error details.
   */
  void onImagesToPdfFinished(const ImagesToPdfController::Result &result);

private:
  void moveCurrentItem(int delta);

  ImagesToPdfController *m_controller{nullptr};
  QLabel *m_hintLabel{nullptr};
  QListWidget *m_orderList{nullptr};
  ui::Button *m_syncBtn{nullptr};
  ui::Button *m_upBtn{nullptr};
  ui::Button *m_downBtn{nullptr};
  ui::Select *m_pageSizeSelect{nullptr};
  ui::FormField *m_pageSizeField{nullptr};
  ui::Select *m_orientationSelect{nullptr};
  ui::FormField *m_orientationField{nullptr};
  ui::Select *m_marginSelect{nullptr};
  ui::FormField *m_marginField{nullptr};
};

#endif // IMAGES_TO_PDF_PANEL_H
