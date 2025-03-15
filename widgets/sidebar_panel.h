#ifndef LEFT_PANEL_H
#define LEFT_PANEL_H

#include "../widgets/button_action.h"
#include "button_toggle.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

class SidebarPanel : public QWidget {
  Q_OBJECT

public:
  explicit SidebarPanel(QWidget *parent = nullptr);
public slots:
  void emitNavigateSignal(int pageIndex);

signals:
  void navigateToPage(int pageIndex);

private:
  bool m_isCollapsed = false;
  int m_currentIndex = -1;
  ButtonToggle *m_buttonToggle;
  QVBoxLayout *m_buttonLayout;
  QVBoxLayout *m_runButtonLayout;

  int getCurrentIndex() const { return m_currentIndex; }
  void setCurrentIndex(int index) { m_currentIndex = index; }
  void createRunButton();
  void setupInputPageButtons();
  void setupOutputPageButtons();
  void updateButtonState(QFuture<void> future, ButtonAction *runButton,
                         ButtonAction *staticRunButton);
};

#endif // LEFT_PANEL_H
