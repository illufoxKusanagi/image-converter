#ifndef SPLIT_PDF_PANEL_H
#define SPLIT_PDF_PANEL_H

#include "controllers/pdf/split_controller.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/number_input.h"
#include "ui-kit/components/select.h"
#include <QString>
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for configuring PDF splitting modes and parameters.
 *
 * Provides controls to select the split strategy (custom page ranges, fixed
 * page interval, or extracting every page), enables only the inputs
 * applicable to the active mode, and owns the SplitController used to run
 * split jobs.
 */
class SplitPdfPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Split PDF options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit SplitPdfPanel(QWidget *parent = nullptr);
  ~SplitPdfPanel() override = default;

  /**
   * @brief Returns the active split mode index.
   * @return 0 for custom page ranges, 1 for fixed interval, 2 for all pages.
   */
  int splitMode() const;

  /**
   * @brief Returns the trimmed custom page range specification.
   * @return User-entered 1-based page range string (e.g. "1-3, 5, 8-12").
   */
  QString rangeSpec() const;

  /**
   * @brief Returns the fixed interval page count.
   * @return Positive interval size (clamped to at least 1).
   */
  int interval() const;

  /**
   * @brief Validates split inputs, selects an output folder, and starts splitting.
   * @param sourcePaths Selected PDF file paths to split.
   * @return True if the split job was started; false on validation failure or cancel.
   */
  bool execute(const QStringList &sourcePaths);

  /**
   * @brief Returns the split job controller owned by this panel.
   * @return Pointer to the panel's SplitController as PdfToolController.
   */
  PdfToolController *controller() const { return m_controller; }

signals:
  /**
   * @brief Emitted when an active split operation finishes or is cancelled.
   */
  void executionFinished();

public slots:
  /**
   * @brief Updates input enabled states for the selected split mode.
   * @param index Active split mode index from the mode selector.
   */
  void onModeChanged(int index);

private slots:
  /**
   * @brief Handles split job completion and presents result feedback.
   * @param result Outcome summary emitted by SplitController.
   */
  void onSplitFinished(const SplitController::Result &result);

private:
  ui::Select *m_modeSelect{nullptr};
  ui::FormField *m_modeField{nullptr};
  ui::Input *m_rangesInput{nullptr};
  ui::FormField *m_rangesField{nullptr};
  ui::NumberInput *m_intervalInput{nullptr};
  ui::FormField *m_intervalField{nullptr};
  SplitController *m_controller{nullptr};
};

#endif // SPLIT_PDF_PANEL_H
