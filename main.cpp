#include "mainwindow/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QCoreApplication::setOrganizationName("Illufox Kasunagi");
  QCoreApplication::setApplicationName("Image Converter");
  QIcon appIcon;
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(64, 64));
  app.setWindowIcon(appIcon);
  app.setStyleSheet("QWidget { background-color: white; color: black;}");
  MainWindow w;
  w.show();
  return app.exec();
}
