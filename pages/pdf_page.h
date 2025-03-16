#ifndef PDF_PAGE_H
#define PDF_PAGE_H
#include "widgets/input_widget.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QStackedWidget>

class PdfPage : public QWidget {
  Q_OBJECT
public:
  explicit PdfPage(QWidget *parent = nullptr);

private:
  QVBoxLayout *mainLayout;
};

#endif // PDF_PAGE_H
