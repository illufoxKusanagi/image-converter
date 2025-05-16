#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  stackedWidget = new QStackedWidget(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  MainPage *mainPage = new MainPage(this);
  // SidebarPanel *sidebarPanel = new SidebarPanel(this);
  PdfPage *pdfPage = new PdfPage(this);
  // mainLayout->addWidget(sidebarPanel);
  stackedWidget->addWidget(mainPage);
  stackedWidget->addWidget(pdfPage);
  mainLayout->addWidget(stackedWidget);
  // connect(sidebarPanel, &SidebarPanel::navigateToPage, this,
  //         &MainWindow::navigateToPage);
  setCentralWidget(centralWidget);
  navigateToPage(0);
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint |
                 Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
}

void MainWindow::navigateToPage(int pageIndex) {
  if (pageIndex >= 0 && pageIndex < stackedWidget->count()) {
    stackedWidget->setCurrentIndex(pageIndex);
  }
}