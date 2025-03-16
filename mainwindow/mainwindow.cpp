#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setMinimumSize(800, 600);
  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  MainPage *mainPage = new MainPage(this);
  SidebarPanel *sidebarPanel = new SidebarPanel(this);
  mainLayout->addWidget(sidebarPanel);
  mainLayout->addWidget(mainPage);
  setCentralWidget(centralWidget);
}
