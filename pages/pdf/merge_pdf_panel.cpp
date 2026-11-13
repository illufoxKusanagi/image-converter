#include "merge_pdf_panel.h"
#include <qboxlayout.h>

MergePdfPanel::MergePdfPanel(QObject *parent) : QObject{parent} {
  // Page 1: Merge PDF
  m_mergePanel = new QWidget(m_optionsStack);
  QVBoxLayout *mergeLayout = new QVBoxLayout(m_mergePanel);
  mergeLayout->setContentsMargins(0, 0, 0, 0);
  mergeLayout->setSpacing(6);

  QHBoxLayout *mergeHeaderLayout = new QHBoxLayout();
  mergeHeaderLayout->setContentsMargins(0, 0, 0, 0);
  mergeHeaderLayout->setSpacing(8);

  m_mergeHintLabel = new QLabel(
      "Merge Order (drag items or use buttons to reorder):", m_mergePanel);
  m_mergeSyncBtn =
      new ui::Button("Sync from Drop Zone", ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Small, m_mergePanel);
  m_mergeUpBtn = new ui::Button("Move Up", ui::ButtonVariant::Outline,
                                ui::ButtonSize::Small, m_mergePanel);
  m_mergeDownBtn = new ui::Button("Move Down", ui::ButtonVariant::Outline,
                                  ui::ButtonSize::Small, m_mergePanel);

  connect(m_mergeSyncBtn, &QPushButton::clicked, this,
          &PdfPage::syncMergeListFromDropWidget);
  connect(m_mergeUpBtn, &QPushButton::clicked, this, &PdfPage::moveMergeItemUp);
  connect(m_mergeDownBtn, &QPushButton::clicked, this,
          &PdfPage::moveMergeItemDown);

  mergeHeaderLayout->addWidget(m_mergeHintLabel, 1);
  mergeHeaderLayout->addWidget(m_mergeSyncBtn);
  mergeHeaderLayout->addWidget(m_mergeUpBtn);
  mergeHeaderLayout->addWidget(m_mergeDownBtn);

  m_mergeOrderList = new QListWidget(m_mergePanel);
  m_mergeOrderList->setDragDropMode(QAbstractItemView::InternalMove);
  m_mergeOrderList->setDefaultDropAction(Qt::MoveAction);
  m_mergeOrderList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_mergeOrderList->setFixedHeight(115);

  mergeLayout->addLayout(mergeHeaderLayout);
  mergeLayout->addWidget(m_mergeOrderList);
  m_optionsStack->addWidget(m_mergePanel);
}
