#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Main Window");
  setMinimumSize(800, 600);

  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  MainPage *mainPage = new MainPage(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  layout->addWidget(mainPage);
}
