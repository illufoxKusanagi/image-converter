#ifndef MainWindowDummy_H
#define MainWindowDummy_H

#include "pages/main_page.h"
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

class MainWindowDummy : public QMainWindow {
  Q_OBJECT

public:
  MainWindowDummy(QWidget *parent = nullptr);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private slots:
  void convertImage();

private:
  QLabel *label;
  QPushButton *button;
  QSlider *qualitySlider;
  QSpinBox *qualityValue;
  void processImage(const QString &filePath);
};

#endif // MainWindowDummy_H
