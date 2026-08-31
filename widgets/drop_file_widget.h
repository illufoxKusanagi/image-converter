#ifndef DROPFILEWIDGET_H
#define DROPFILEWIDGET_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include "widgets/button_action.h"
#include "widgets/message_box_widget.h"
#include "widgets/slider_widget.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class DropFileWidget : public QGroupBox {
  Q_OBJECT
public:
  enum ImageExtension { JPG, JPEG, PNG, WEBP, TIFF, BMP, GIF, PDF };
  explicit DropFileWidget(QWidget *parent = nullptr, QString typeFile = "File",
                          SliderWidget *sliderWidget = nullptr,
                          ImageExtension *sourceExtension = nullptr);
  QString getFilePath() const;
  QString filePath() const { return getFilePath(); }
  QStringList getFilePaths() const;
  QStringList filePaths() const { return getFilePaths(); }
  void clearFiles();
  void removeFileAt(int index);
  void moveFileUp(int index);
  void moveFileDown(int index);
  QString imageExtensionToString(const ImageExtension &sourceExtension) const;
  bool saveImage(const QImage *image, const QString &outputPath,
                 const int quality, const ImageExtension *sourceExtension);
  static QString formatFileSize(qint64 bytes);

public slots:
  void convertImage(const QString &sourcePath);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private slots:
  void onBrowseButtonPressed();
  void onSliderValueChanged();
  void onClearAllPressed();

private:
  QVBoxLayout *mainLayout;
  SliderWidget *m_sliderWidget;
  ImageExtension *m_sourceExtension;
  ButtonAction *m_browseButton;
  QLabel *m_icon;
  int m_qualityValue;
  QString m_sourcePath;
  QStringList m_filePaths;
  QString m_typeFile;
  bool m_isDragHovered;

  QWidget *m_emptyFieldWidget;
  QWidget *m_chosenFileWidget;
  QLabel *m_chosenHeaderLabel;
  QPushButton *m_clearAllButton;
  QPushButton *m_addMoreButton;
  QScrollArea *m_fileScrollArea;
  QWidget *m_fileListContainer;
  QVBoxLayout *m_fileListLayout;

  void updateDropZoneStyle();
  void setupOrSeparatorLayout(QVBoxLayout *layout);
  void setupEmptyFileWidget();
  void setupChosenFileWidget();
  void updateWidgetVisibility();
  void rebuildFileList();
  QPixmap createColoredIcon(const QString &iconPath, const QColor &color,
                            int width, int height);
};

#endif // DROPFILEWIDGET_H
