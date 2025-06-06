#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  QTabWidget *tabWidget = new QTabWidget(this);
  mainLayout->setContentsMargins(32, 32, 32, 32);
  MainPage *mainPage = new MainPage(this);
  PdfPage *pdfPage = new PdfPage(this);
  tabWidget->addTab(mainPage, "Convert Image");
  tabWidget->addTab(pdfPage, "Compress PDF");
  // TODO : separating the styles to a specific method
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
      "QTabBar::tab:selected:hover {"
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
  setCentralWidget(centralWidget);
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint |
                 Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
}