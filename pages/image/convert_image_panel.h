#ifndef CONVERT_IMAGE_PANEL_H
#define CONVERT_IMAGE_PANEL_H

#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QImage>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <atomic>
#include <functional>

/**
 * @brief Options panel and execution engine for Mode 0 (Convert Image).
 *
 * Provides target format selection and quality configuration controls, and
 * orchestrates asynchronous conversion for Images -> PDF, PDF -> Images, and
 * single/batch Image or SVG -> Image workflows.
 */
class ConvertImagePanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Convert Image options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit ConvertImagePanel(QWidget *parent = nullptr);
  ~ConvertImagePanel() override;

  /**
   * @brief Loads a raster image or renders an SVG file into a QImage.
   * @param path Source file path.
   * @param forceOpaqueWhite Whether alpha channels should be composited onto
   * white.
   * @return Loaded QImage, or a null QImage on failure.
   */
  static QImage loadRasterOrSvgImage(const QString &path,
                                     bool forceOpaqueWhite = false);

  /**
   * @brief Returns the owned image quality slider widget.
   * @return Pointer to the SliderWidget instance.
   */
  SliderWidget *qualitySlider() const;

  /**
   * @brief Returns a pointer to the active target image extension enum.
   * @return Pointer to m_targetExtension for DropFileWidget binding.
   */
  DropFileWidget::ImageExtension *targetExtensionPtr();

  /**
   * @brief Synchronizes control enabled and title state with active job status.
   * @param isProcessing True while an asynchronous conversion job is running.
   */
  void syncProcessingState(bool isProcessing);

  /**
   * @brief Validates inputs, prompts for output destination, and starts job.
   * @param dragWidget Active DropFileWidget supplying input paths and saving.
   * @return True if async execution started; false on validation failure or
   * cancelled file dialog.
   */
  bool execute(DropFileWidget *dragWidget);

  /**
   * @brief Requests cancellation of any running conversion job.
   */
  void cancel();

  /**
   * @brief Checks whether an asynchronous conversion job is currently running.
   * @return True if the internal QFutureWatcher is running.
   */
  bool isRunning() const;

signals:
  /**
   * @brief Emitted right before starting the watcher to configure progress UI.
   * @param min Minimum progress range value.
   * @param max Maximum progress range value.
   * @param indeterminate True for indeterminate progress animation.
   * @param initialText Initial status label text.
   */
  void progressConfigured(int min, int max, bool indeterminate,
                          const QString &initialText);

  /**
   * @brief Emitted as background workers report progress updates.
   * @param completed Number of completed items/pages.
   * @param total Total number of items/pages.
   * @param labelText Updated status label text (empty preserves existing text).
   */
  void progressUpdated(int completed, int total, const QString &labelText);

  /**
   * @brief Emitted at the start of the watcher finished callback before dialogs.
   */
  void executionFinished();

private slots:
  void onExtensionChanged(int index);
  void onFutureFinished();

private:
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_targetExtensionSelect{nullptr};
  ui::FormField *m_targetExtensionField{nullptr};
  DropFileWidget::ImageExtension m_targetExtension{
      DropFileWidget::ImageExtension::JPG};
  QFutureWatcher<void> *m_futureWatcher{nullptr};
  std::atomic<bool> m_isCancelled{false};
  std::function<void()> m_pendingCompletion;

  bool executeImagesToPdf(DropFileWidget *dragWidget,
                          const QStringList &sourcePaths, int quality);
  bool executePdfToImages(DropFileWidget *dragWidget,
                          const QStringList &sourcePaths, int quality,
                          DropFileWidget::ImageExtension targetExt,
                          const QString &targetFormatString);
  bool executeSingleImageConvert(DropFileWidget *dragWidget,
                                 const QString &sourcePath, int quality,
                                 DropFileWidget::ImageExtension targetExt,
                                 const QString &targetFormatString,
                                 bool forceOpaque);
  bool executeBatchImageConvert(DropFileWidget *dragWidget,
                                const QStringList &sourcePaths, int quality,
                                DropFileWidget::ImageExtension targetExt,
                                const QString &targetFormatString,
                                bool forceOpaque);

  void postProgress(int completed, int total, const QString &labelText);
  static void postFileStatus(DropFileWidget *dragWidget,
                             const QPointer<DropFileWidget> &safeDragWidget,
                             const QString &path,
                             DropFileWidget::FileStatus status,
                             const QString &detail);
};

#endif // CONVERT_IMAGE_PANEL_H
