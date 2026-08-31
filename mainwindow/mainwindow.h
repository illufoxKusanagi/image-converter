#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/main_page.h"
#include "pages/pdf_page.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/tabs.h"
#include "ui-kit/theme/theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

private slots:
  void applyThemeStyles();
  void onThemeToggleClicked();

private:
  ui::Tabs *m_tabs;
  ui::Button *m_themeToggleBtn;
  QLabel *m_titleLabel;
  QWidget *m_centralWidget;
  MainPage *m_mainPage;
};

#endif // MAINWINDOW_H
