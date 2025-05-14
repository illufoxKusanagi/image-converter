#ifndef PDF_PAGE_H
#define PDF_PAGE_H
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QStackedWidget>


/**
 * @brief Constructs a PdfPage widget with an optional parent.
 *
 * Initializes the PdfPage, which serves as a container for PDF-related input and display widgets.
 *
 * @param parent Optional parent widget.
 */
class PdfPage : public QWidget {
  Q_OBJECT
public:
  explicit PdfPage(QWidget *parent = nullptr);

private:
  QVBoxLayout *mainLayout;
  void setupImageInput();
};

#endif // PDF_PAGE_H
