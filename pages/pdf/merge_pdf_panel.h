#pragma once

#include "components/button.h"
#include <QObject>
#include <qlabel.h>
#include <qlistwidget.h>
#include <qstackedwidget.h>

class MergePdfPanel : public QObject {
  Q_OBJECT

public:
  explicit MergePdfPanel(QObject *parent = nullptr);
  QWidget *m_mergePanel{nullptr};
  QLabel *m_mergeHintLabel{nullptr};
  QListWidget *m_mergeOrderList{nullptr};
  ui::Button *m_mergeSyncBtn{nullptr};
  ui::Button *m_mergeUpBtn{nullptr};
  ui::Button *m_mergeDownBtn{nullptr};
  QStackedWidget *m_optionsStack{nullptr};

signals:
};
