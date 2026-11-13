#ifndef SPLIT_PDF_PANEL_H
#define SPLIT_PDF_PANEL_H

#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/number_input.h"
#include "ui-kit/components/select.h"
#include <QString>
#include <QWidget>

/**
 * @brief Options panel for configuring PDF splitting modes and parameters.
 *
 * Provides controls to select the split strategy (custom page ranges, fixed
 * page interval, or extracting every page) and enables only the inputs
 * applicable to the active mode.
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

public slots:
  /**
   * @brief Updates input enabled states for the selected split mode.
   * @param index Active split mode index from the mode selector.
   */
  void onModeChanged(int index);

private:
  ui::Select *m_modeSelect{nullptr};
  ui::FormField *m_modeField{nullptr};
  ui::Input *m_rangesInput{nullptr};
  ui::FormField *m_rangesField{nullptr};
  ui::NumberInput *m_intervalInput{nullptr};
  ui::FormField *m_intervalField{nullptr};
};

#endif // SPLIT_PDF_PANEL_H
