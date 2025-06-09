#ifndef PDF_PAGE_H
#define PDF_PAGE_H
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
#include "widgets/slider_widget.h"
#include <QGridLayout>
#include <QGroupBox>
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
  double calculateRenderDPI(int quality); // Kept for compatibility
};

#endif // PDF_PAGE_H
