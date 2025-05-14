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
  explicit DropFileWidget(QWidget *parent = nullptr, QString typeFile = "File",
                          SliderWidget *sliderWidget = nullptr);
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
  void setupOrSeparatorLayout();
  int m_qualityValue;
  QString m_sourcePath;
  QPixmap createColoredIcon(const QString &iconPath, const QColor &color,
                            int width, int height);
};

#endif // DROPFILEWIDGET_H
