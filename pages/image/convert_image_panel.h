#ifndef CONVERT_IMAGE_PANEL_H
#define CONVERT_IMAGE_PANEL_H

#include "controllers/image/image_convert_controller.h"
#include "controllers/pdf/pdf_tool_controller.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QImage>
#include <QString>
#include <QStringList>
#include <QWidget>

/**
 * @brief Options panel for Mode 0 (Convert Image).
 *
 * Provides target format selection and quality configuration controls, and
 * delegates asynchronous conversion to ImageConvertController.
 */
class ConvertImagePanel : public QWidget {
  Q_OBJECT

public:
  explicit ConvertImagePanel(QWidget *parent = nullptr);
  ~ConvertImagePanel() override = default;

  static QImage loadRasterOrSvgImage(const QString &path,
                                     bool forceOpaqueWhite = false);

  void syncProcessingState(bool isProcessing);
  bool execute(const QStringList &sourcePaths);
  PdfToolController *controller() const { return m_controller; }

signals:
  void executionFinished();

private slots:
  void onExtensionChanged(int index);
  void onConvertFinished(const ImageConvertController::Result &result);

private:
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_targetExtensionSelect{nullptr};
  ui::FormField *m_targetExtensionField{nullptr};
  DropFileWidget::ImageExtension m_targetExtension{
      DropFileWidget::ImageExtension::JPG};
  ImageConvertController *m_controller{nullptr};
};

#endif // CONVERT_IMAGE_PANEL_H
