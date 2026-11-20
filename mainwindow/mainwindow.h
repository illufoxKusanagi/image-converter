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
#include <QPixmap>
#include <QVBoxLayout>

class QResizeEvent;
class QTimer;
class QVariantAnimation;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void applyThemeStyles();
  void onThemeToggleClicked();
  void onLanguageToggleClicked();
  void onTrakteerClicked();
  void onKofiClicked();
  void retranslateUi();

private:
  void triggerTrakteerWiggle();
  void updateTrakteerBadgeIcon(qreal angleDegrees);
  void scheduleNextSupportCallout();
  void showSupportCallout();
  void hideSupportCallout(bool immediate = false);
  void positionSupportCallout();

  ui::Tabs *m_tabs{nullptr};
  ui::Button *m_trakteerBtn{nullptr};
  ui::Button *m_kofiBtn{nullptr};
  ui::Button *m_langToggleBtn{nullptr};
  ui::Button *m_themeToggleBtn{nullptr};
  QLabel *m_titleLabel{nullptr};
  QWidget *m_centralWidget{nullptr};
  MainPage *m_mainPage{nullptr};
  PdfPage *m_pdfPage{nullptr};
  QPixmap m_trakteerBadgePixmap;
  QVariantAnimation *m_trakteerWiggleAnim{nullptr};
  QTimer *m_trakteerWiggleTimer{nullptr};
  QWidget *m_supportCallout{nullptr};
  QVariantAnimation *m_supportCalloutAnim{nullptr};
  QTimer *m_supportCalloutDelayTimer{nullptr};
  QTimer *m_supportCalloutHideTimer{nullptr};
  int m_supportCalloutShownCount{0};
  bool m_supportCalloutSuppressed{false};
  bool m_supportCalloutHiding{false};
};

#endif // MAINWINDOW_H
