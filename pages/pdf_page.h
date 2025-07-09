#ifndef PDF_PAGE_H
#define PDF_PAGE_H
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
#include "widgets/slider_widget.h"
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QPageSize>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QProcess>
#include <QProgressDialog>
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
  bool compressPdf(const QString &inputPath, const QString &outputPath,
                   int quality);
  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  int calculateOptimalDPI(int quality);
  int calculateOptimalScaleFactor(int quality);
  qreal calculateOptimalScale(int quality);
  QImage optimizeImageCompression(const QImage &originalImage, int quality);
  QImage optimizePageImage(const QImage &image, int quality);
  qreal calculateOptimalScale(int quality, const QSizeF &pageSize);
  QImage applySmartCompression(const QImage &image, int quality);
  QImage applyMinimalCompression(const QImage &image, int quality);
  QImage simpleCompress(const QImage &image, int quality);
  QImage removeBorderArtifacts(const QImage &image, int quality);
  QImage cleanImageBorders(const QImage &image);
  QImage cleanBordersOnly(const QImage &image);
  bool testMutool(const QString &inputPath, const QString &outputPath,
                  int quality);
  bool tryMutoolStrategy(const QString &input, const QString &output,
                         const QStringList &baseArgs);
  bool compressPdfAdvanced(const QString &inputPath, const QString &outputPath,
                           int quality);
  QStringList getCompressionStrategies(int quality);
  bool executeCompressionStrategy(const QString &input, const QString &output,
                                  const QString &strategy);
  bool fallbackCompression(const QString &inputPath, const QString &outputPath,
                           int quality);
  void cleanupTempFiles(const QString &outputPath, int maxIndex);
};

#endif // PDF_PAGE_H
