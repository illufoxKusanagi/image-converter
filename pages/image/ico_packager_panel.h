#ifndef ICO_PACKAGER_PANEL_H
#define ICO_PACKAGER_PANEL_H

#include "controllers/image/ico_export_controller.h"
#include "ui-kit/components/switch.h"
#include <QLabel>
#include <QStringList>
#include <QVector>
#include <QWidget>

/**
 * @brief Options panel for configuring multi-resolution App Icon (.ico) export.
 *
 * Provides resolution toggles (16x16 through 256x256 px) and owns the
 * IcoExportController that packages raster and SVG inputs into .ico files.
 */
class IcoPackagerPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the App Icon (.ico) packager options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit IcoPackagerPanel(QWidget *parent = nullptr);
  ~IcoPackagerPanel() override = default;

  /**
   * @brief Returns the enabled icon resolutions in pixels.
   * @return List of checked icon sizes (subset of 16, 32, 48, 64, 128, 256).
   */
  QVector<int> selectedSizes() const;

  /**
   * @brief Validates inputs and icon sizes, prompts for an output directory,
   * and starts .ico export.
   * @param sourcePaths List of source image or SVG file paths to package.
   * @return True if export started; false on validation failure or cancel.
   */
  bool execute(const QStringList &sourcePaths);

  /**
   * @brief Returns the underlying tool controller owned by this panel.
   * @return Pointer to the owned IcoExportController as PdfToolController.
   */
  PdfToolController *controller() const { return m_controller; }

public slots:
  /**
   * @brief Applies current theme typography and colors to panel labels.
   */
  void applyThemeStyles();

signals:
  /**
   * @brief Emitted when the background .ico export job finishes or is
   * cancelled.
   */
  void executionFinished();

private slots:
  void onIcoExportFinished(const IcoExportController::Result &result);

private:
  struct IcoSizeToggle {
    int size{0};
    ui::Switch *toggle{nullptr};
    QLabel *label{nullptr};
  };

  QLabel *m_hintLabel{nullptr};
  QVector<IcoSizeToggle> m_sizeToggles;
  IcoExportController *m_controller{nullptr};
};

#endif // ICO_PACKAGER_PANEL_H
