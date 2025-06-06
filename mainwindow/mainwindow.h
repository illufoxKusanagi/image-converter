#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/main_page.h"
#include "pages/pdf_page.h"
#include "styles/colors.h"
#include "styles/text_style.h"
#include "widgets/sidebar_panel.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>


class MainWindow : public QMainWindow {
  Q_OBJECT

  // private slots:
  //   void navigateToPage(int pageIndex);

public:
  MainWindow(QWidget *parent = nullptr);

private:
  QStackedWidget *stackedWidget;
};

#endif // MAINWINDOW_H
