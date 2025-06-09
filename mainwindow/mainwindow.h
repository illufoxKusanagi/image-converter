#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/main_page.h"
#include "pages/pdf_page.h"
#include "styles/colors.h"
#include "styles/text_style.h"
#include <QFileDialog>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

private:
  QTabWidget *m_tabWidget;
  void setupTabStyle();
};

#endif // MAINWINDOW_H
