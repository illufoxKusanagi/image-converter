#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/main_page.h"
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
#include <QVBoxLayout>


class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

private:
};

#endif // MAINWINDOW_H
