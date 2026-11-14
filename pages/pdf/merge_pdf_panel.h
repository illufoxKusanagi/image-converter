#ifndef MERGE_PDF_PANEL_H
#define MERGE_PDF_PANEL_H

#include "controllers/pdf/merge_controller.h"
#include "ui-kit/components/button.h"
#include <QLabel>
#include <QListWidget>
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for configuring the PDF merge order.
 *
 * Displays an interactive reorderable list of selected PDF files synced from
 * the drop zone, along with controls to synchronize or move items up and down.
 */
class MergePdfPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Merge PDF options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit MergePdfPanel(QWidget *parent = nullptr);
  ~MergePdfPanel() override = default;

  /**
   * @brief Synchronizes the list with the current drop-zone file paths.
   *
   * Preserves existing item ordering for paths still present in @p dropPaths,
   * removes paths no longer present, and appends newly added unique paths.
   *
   * @param dropPaths Current list of file paths from the drop widget.
   */
  void syncFromPaths(const QStringList &dropPaths);

  /**
   * @brief Returns the file paths in their current visual merge order.
   * @return Ordered list of full file paths stored in each item's Qt::UserRole.
   */
  QStringList orderedPaths() const;

  /**
   * @brief Returns the number of files currently listed in the merge order.
   */
  int count() const;

  /**
   * @brief Validates selected PDFs, prompts for output path, and starts merge.
   * @param dropPaths Current list of file paths from the drop widget.
   * @return True if the asynchronous merge job was started; false otherwise.
   */
  bool execute(const QStringList &dropPaths);

  /**
   * @brief Returns the underlying controller managing the merge operation.
   */
  PdfToolController *controller() const { return m_controller; }

public slots:
  /**
   * @brief Applies current theme colors, radii, and typography to child widgets.
   */
  void applyThemeStyles();

  /**
   * @brief Moves the currently selected file one position up in merge order.
   */
  void moveItemUp();

  /**
   * @brief Moves the currently selected file one position down in merge order.
   */
  void moveItemDown();

signals:
  /**
   * @brief Emitted when the user clicks "Sync from Drop Zone".
   */
  void syncRequested();

  /**
   * @brief Emitted when the asynchronous merge job finishes or is cancelled.
   */
  void executionFinished();

private slots:
  void onMergeFinished(const MergeController::Result &result);

private:
  void moveCurrentItem(int delta);

  QLabel *m_hintLabel{nullptr};
  QListWidget *m_orderList{nullptr};
  ui::Button *m_syncBtn{nullptr};
  ui::Button *m_upBtn{nullptr};
  ui::Button *m_downBtn{nullptr};
  MergeController *m_controller{nullptr};
};

#endif // MERGE_PDF_PANEL_H
