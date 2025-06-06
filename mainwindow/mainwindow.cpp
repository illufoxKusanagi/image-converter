#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  QTabWidget *tabWidget = new QTabWidget(this);
  mainLayout->setContentsMargins(32, 32, 32, 32);
  // stackedWidget = new QStackedWidget(this);
  // SidebarPanel *sidebarPanel = new SidebarPanel(this);
  MainPage *mainPage = new MainPage(this);
  PdfPage *pdfPage = new PdfPage(this);
  // mainLayout->addWidget(sidebarPanel);
  // stackedWidget->addWidget(mainPage);
  // stackedWidget->addWidget(pdfPage);
  // mainLayout->addWidget(stackedWidget);
  tabWidget->addTab(mainPage, "Convert Image");
  tabWidget->addTab(pdfPage, "Compress PDF");
  tabWidget->setStyleSheet(
      "QTabWidget::pane {"
      "  border: 1px solid #d1d5db;"
      "  border-top: none;"
      "  border-radius: 0 0 8px 8px;"
      "  background: #ffffff;"
      "}"
      "QTabBar::tab {"
      "  background: #f5f6fa;"
      "  border: 1px solid #d1d5db;"
      "  border-bottom: none;"
      "  min-width: 135px;"
      "  min-height: 32px;"
      "  padding: 8px 20px;" +
      TextStyle::BodyMediumRegular() + "color:" + Colors::StandardBlack.name() +
      ";"
      "  border-top-left-radius: 8px;"
      "  border-top-right-radius: 8px;"
      "  margin-right: 2px;"
      "  margin-bottom: -1px;"
      "}"
      "QTabBar::tab:selected {"
      "  background: #ffffff;" +
      TextStyle::BodyMediumBold() + " color: " + Colors::Secondary500.name() +
      ";"
      "  border-bottom: 1px solid #ffffff;"
      "}"
      "QTabBar::tab:selected:hover {" // <--- Add this block
      "  background: #ffffff;" +
      TextStyle::BodyMediumBold() + " color: " + Colors::Secondary500.name() +
      ";"
      "}"
      "QTabBar::tab:!selected {"
      "  margin-top: 2px;"
      "}"
      "QTabBar::tab:hover {" +
      TextStyle::BodyMediumRegular() +
      " color: " + Colors::Secondary500.name() +
      ";"
      "}");
  mainLayout->addWidget(tabWidget);
  // connect(sidebarPanel, &SidebarPanel::navigateToPage, this,
  //         &MainWindow::navigateToPage);
  setCentralWidget(centralWidget);
  // navigateToPage(0);
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint |
                 Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
}

// void MainWindow::navigateToPage(int pageIndex) {
//   if (pageIndex >= 0 && pageIndex < stackedWidget->count()) {
//     stackedWidget->setCurrentIndex(pageIndex);
//   }
// }