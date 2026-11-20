#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/main_page.h"
#include "pages/pdf_page.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/tabs.h"
#include "ui-kit/theme/i18n.h"
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
  void onLanguageToggleClicked();
  void onTrakteerClicked();
  void onKofiClicked();
  void retranslateUi();

private:
  ui::Tabs *m_tabs{nullptr};
  ui::Button *m_trakteerBtn{nullptr};
  ui::Button *m_kofiBtn{nullptr};
  ui::Button *m_langToggleBtn{nullptr};
  ui::Button *m_themeToggleBtn{nullptr};
  QLabel *m_titleLabel{nullptr};
  QWidget *m_centralWidget{nullptr};
  MainPage *m_mainPage{nullptr};
  PdfPage *m_pdfPage{nullptr};
};

#endif // MAINWINDOW_H
