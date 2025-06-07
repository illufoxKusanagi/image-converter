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
  void processPdfFiles(const QStringList &filePaths, int quality);
};

#endif // PDF_PAGE_H
