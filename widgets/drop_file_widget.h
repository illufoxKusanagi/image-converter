#ifndef DROPFILEWIDGET_H
#define DROPFILEWIDGET_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include "widgets/button_action.h"
#include "widgets/slider_widget.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

class DropFileWidget : public QGroupBox {
  Q_OBJECT
public:
  enum ImageExtension { JPG, JPEG, PNG, WEBP, TIFF, PDF };
  explicit DropFileWidget(QWidget *parent = nullptr, QString typeFile = "File",
                          SliderWidget *sliderWidget = nullptr,
                          ImageExtension *sourceExtension = nullptr);
  QString getFilePath();

public slots:
  void convertImage(const QString sourcePath);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private slots:
  void onBrowseButtonPressed();
  void onSliderValueChanged();
  // void compressPdf();

private:
  QVBoxLayout *mainLayout;
  SliderWidget *m_sliderWidget;
  ImageExtension *m_sourceExtension;
  int m_qualityValue;
  QString m_sourcePath;
  void setupOrSeparatorLayout();
  bool saveImage(const QImage *image, const QString &outputPath,
                 const int quality, const ImageExtension *sourceExtension);
  QPixmap createColoredIcon(const QString &iconPath, const QColor &color,
                            int width, int height);
};

#endif // DROPFILEWIDGET_H
