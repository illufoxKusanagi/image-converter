#include "pdf_page.h"

PdfPage::PdfPage(QWidget *parent) : QWidget(parent) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(32);
  QLabel *title = new QLabel("muhahahaa", this);
  title->setStyleSheet(TextStyle::Heading1());
  mainLayout->addWidget(title, 0, Qt::AlignCenter);
  // DropFileWidget *dragWidget = new DropFileWidget(this, "PDF");
  // mainLayout->addWidget(dragWidget);
  // QWidget *dragWidget = new QWidget(this);
  // QVBoxLayout *dragLayout = new QVBoxLayout(dragWidget);
  // dragLayout->setContentsMargins(0, 0, 0, 0);
  // dragWidget->setStyleSheet("background-color: " +
  // Colors::Secondary400.name() +
  //                           ";"
  //                           "border: 2px dashed " +
  //                           Colors::Secondary800.name() + ";");
  // QLabel *dragLabel = new QLabel("Seret dan lepas file di sini", this);
  // dragLabel->setAlignment(Qt::AlignCenter);
  // QPushButton *browseButton = new QPushButton("Pilih File pdf", this);
  // connect(browseButton, &QPushButton::clicked, this,
  // &PdfPage::setupImageInput); dragLayout->addWidget(dragLabel);

  // InputType inputType = InputType("field", "contoh");
  // InputWidget *inputWidget = new InputWidget(inputType, this);
  // mainLayout->addWidget(dragWidget);
  // mainLayout->addWidget(browseButton);
  // mainLayout->addWidget(inputWidget);
  setLayout(mainLayout);
  // setStyleSheet("QWidget {"
  //               "    background-color: " +
  //               Colors::Secondary600.name() +
  //               ";"
  //               "}");
}

void PdfPage::setupImageInput() { qDebug() << "Button clicked"; }