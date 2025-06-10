#ifndef PDF_PAGE_H
#define PDF_PAGE_H
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
#include "widgets/slider_widget.h"
#include <QBuffer>
#include <QGridLayout>
#include <QGroupBox>
#include <QPageSize>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QStackedWidget>

class PdfPage : public QWidget {
  Q_OBJECT
public:
  explicit PdfPage(QWidget *parent = nullptr);

private slots:
  void onProcessButtonClicked();

private:
  QVBoxLayout *mainLayout;
  SliderWidget *m_qualitySlider;
  DropFileWidget *m_dragWidget;
  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  bool compressPdf(const QString &inputPath, const QString &outputPath,
                   int quality);
  QImage compressImage(const QImage &originalImage, int quality);
  int calculateDPI(int quality);
  qreal calculateScaleFactor(int quality);
  qreal calculateRenderScale(int quality);
  void configureCompressionSettings(QPdfWriter &writer, int quality);
  bool compressPage(QPdfDocument &sourceDoc, QPdfWriter &targetWriter,
                    QPainter &painter, int pageIndex, int quality);
  QSize calculateOptimalRenderSize(const QSizeF &pageSize, int quality);
  QImage smartCompress(const QImage &image, int quality);
  QRect calculateTargetRect(QPdfWriter &writer, const QSizeF &pageSize);
  QRect calculateAspectRatioRect(const QSize &imageSize,
                                 const QRect &targetRect);
};

#endif // PDF_PAGE_H
