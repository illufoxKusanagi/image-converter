#ifndef DROPFILEWIDGET_H
#define DROPFILEWIDGET_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include "widgets/button_action.h"
#include "widgets/message_box_widget.h"
#include "widgets/slider_widget.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
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
  QStringList getFilePaths();
  QString imageExtensionToString(const ImageExtension &sourceExtension) const;
  bool saveImage(const QImage *image, const QString &outputPath,
                 const int quality, const ImageExtension *sourceExtension);

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
  ButtonAction *m_browseButton;
  QLabel *m_icon;
  QLabel *m_chosenIcon;
  int m_qualityValue;
  QString m_sourcePath;
  QStringList m_filePaths;
  QString m_typeFile;
  QWidget *m_emptyFieldWidget;
  QWidget *m_chosenFileWidget;
  QLabel *m_chosenLabel;
  void setupOrSeparatorLayout(QVBoxLayout *layout);
  void setupEmptyFileWidget();
  void setupChosenFileWidget();
  void updateWidgetVisibility();
  QPixmap createColoredIcon(const QString &iconPath, const QColor &color,
                            int width, int height);
};

#endif // DROPFILEWIDGET_H
