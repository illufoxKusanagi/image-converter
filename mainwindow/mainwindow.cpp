#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  mainLayout->setContentsMargins(32, 32, 32, 32);
  m_tabWidget = new QTabWidget(this);
  m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_tabWidget->tabBar()->setUsesScrollButtons(false);
  m_tabWidget->setFixedWidth(352);
  MainPage *mainPage = new MainPage(this);
  PdfPage *pdfPage = new PdfPage(this);
  m_tabWidget->addTab(mainPage, "Convert Image");
  m_tabWidget->addTab(pdfPage, "Compress PDF");
  setupTabStyle();
  mainLayout->addWidget(m_tabWidget);
  setCentralWidget(centralWidget);
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint |
                 Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
}

void MainWindow::setupTabStyle() {
  m_tabWidget->setStyleSheet(
      "QTabWidget::pane {"
      "  border: 1px solid #d1d5db;"
      "  border-top: none;"
      "  border-radius: 0 0 8px 8px;"
      "  background: #ffffff;"
      "}"
      "QTabBar::scroller {"
      "  width: 0px;"
      "}"
      "QTabBar::tab {"
      "  background: #f5f6fa;"
      "  border: 1px solid #d1d5db;"
      "  border-bottom: none;"
      "  min-width: 133px;"
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
}