#include "mainwindow/mainwindow.h"
#include "ui-kit/theme/theme.h"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  QCoreApplication::setOrganizationName("Illufox Kasunagi");
  QCoreApplication::setApplicationName("Image Converter");
  QIcon appIcon;
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(16, 16));
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(32, 32));
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(64, 64));
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(128, 128));
  app.setWindowIcon(appIcon);
  app.setStyleSheet("QWidget { background-color: white; color: black;}");
  MainWindow w;
  w.show();
  return app.exec();
}
