#ifndef ORGANIZE_PDF_PANEL_H
#define ORGANIZE_PDF_PANEL_H

#include "controllers/pdf/organize_controller.h"
#include "ui-kit/components/button.h"
#include <QLabel>
#include <QListWidget>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

/**
 * @brief Options panel for organizing, rotating, reordering, and deleting PDF
 * pages.
 *
 * Displays page thumbnails and rotation metadata for a loaded PDF document,
 * with toolbar actions to load, rotate, reorder, delete, and reset pages.
 */
class OrganizePdfPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Organize PDF options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit OrganizePdfPanel(QWidget *parent = nullptr);
  ~OrganizePdfPanel() override = default;

  /**
   * @brief Loads page previews from the first PDF path in @p paths.
   *
   * Clears existing items and updates @c sourcePath() on success. Displays a
   * warning toast when @p paths is empty unless @p silentIfEmpty is true, and
   * a destructive toast if the PDF cannot be opened.
   *
   * @param paths Candidate PDF file paths (typically from the drop zone).
   * @param silentIfEmpty Suppresses the empty-selection warning toast when true.
   */
  void loadFromPaths(const QStringList &paths, bool silentIfEmpty = false);

  /**
   * @brief Validates @p dropPaths, prompts for an output PDF path, and starts
   * page organization asynchronously.
   *
   * Loads page previews when no matching PDF is loaded, verifies the output
   * path does not overwrite the source PDF, and starts the organize job.
   *
   * @param dropPaths Candidate PDF file paths from the drop zone.
   * @return @c true if the organize job was started; @c false on validation
   * failure or save-dialog cancellation.
   */
  bool execute(const QStringList &dropPaths);

  /**
   * @brief Returns the underlying organize controller owned by this panel.
   */
  PdfToolController *controller() const { return m_controller; }

  /**
   * @brief Returns the ordered list of page operations configured by the user.
   * @return Vector of page operations with original indices and rotations.
   */
  QVector<OrganizeController::PageOp> pageOps() const;

  /**
   * @brief Returns the file path of the currently loaded source PDF.
   */
  QString sourcePath() const;

  /**
   * @brief Returns the number of pages currently retained in the organize list.
   */
  int pageCount() const;

public slots:
  /**
   * @brief Applies current theme colors, radii, and typography to child widgets.
   */
  void applyThemeStyles();

  /**
   * @brief Rotates all currently selected pages by @p deltaDegrees.
   * @param deltaDegrees Signed rotation step in degrees (normalized to 0-359).
   */
  void rotateSelectedPages(int deltaDegrees);

  /**
   * @brief Moves the current page row by @p direction (-1 for up, +1 for down).
   * @param direction Row offset to shift the current item.
   */
  void moveSelectedPage(int direction);

  /**
   * @brief Deletes selected pages while ensuring at least one page remains.
   */
  void deleteSelectedPages();

signals:
  /**
   * @brief Emitted when the user requests loading or resetting from the drop
   * zone PDF.
   */
  void reloadRequested();

  /**
   * @brief Emitted when the organize controller finishes execution.
   */
  void executionFinished();

private slots:
  /**
   * @brief Handles completion of the background organize job and notifies the
   * user.
   * @param result Outcome details from @c OrganizeController.
   */
  void onOrganizeFinished(const OrganizeController::Result &result);

private:
  void updateItemDisplay(QListWidgetItem *item) const;

  QLabel *m_hintLabel{nullptr};
  QListWidget *m_pageList{nullptr};
  ui::Button *m_loadBtn{nullptr};
  ui::Button *m_rotLeftBtn{nullptr};
  ui::Button *m_rotRightBtn{nullptr};
  ui::Button *m_rot180Btn{nullptr};
  ui::Button *m_moveUpBtn{nullptr};
  ui::Button *m_moveDownBtn{nullptr};
  ui::Button *m_deleteBtn{nullptr};
  ui::Button *m_resetBtn{nullptr};
  OrganizeController *m_controller{nullptr};
  QString m_sourcePath;
};

#endif // ORGANIZE_PDF_PANEL_H
