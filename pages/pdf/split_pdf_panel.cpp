#include "split_pdf_panel.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QStringList>
#include <QtGlobal>

SplitPdfPanel::SplitPdfPanel(QWidget *parent) : QWidget(parent) {
  auto *splitLayout = new QHBoxLayout(this);
  splitLayout->setContentsMargins(0, 0, 0, 0);
  splitLayout->setSpacing(12);

  const QStringList splitModes = {"Custom Page Ranges (e.g. 1-3, 5, 8-12)",
                                  "Fixed Interval (Every N Pages)",
                                  "Extract All Pages (1 PDF per page)"};
  m_modeSelect = new ui::Select(splitModes, this);
  m_modeField = new ui::FormField("Split Mode", m_modeSelect, this);
  connect(m_modeSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SplitPdfPanel::onModeChanged);

  m_rangesInput = new ui::Input("e.g. 1-3, 5, 8-12", this);
  m_rangesInput->setText("1");
  m_rangesField =
      new ui::FormField("Page Ranges (1-based)", m_rangesInput, this);

  m_intervalInput = new ui::NumberInput(this);
  m_intervalInput->setRange(1, 1000);
  m_intervalInput->setValue(1);
  m_intervalInput->setSuffix(" page(s)");
  m_intervalField =
      new ui::FormField("Interval Size", m_intervalInput, this);

  splitLayout->addWidget(m_modeField, 2, Qt::AlignTop);
  splitLayout->addWidget(m_rangesField, 2, Qt::AlignTop);
  splitLayout->addWidget(m_intervalField, 1, Qt::AlignTop);

  onModeChanged(0);
}

int SplitPdfPanel::splitMode() const {
  return m_modeSelect ? m_modeSelect->currentIndex() : 0;
}

QString SplitPdfPanel::rangeSpec() const {
  return m_rangesInput ? m_rangesInput->text().trimmed() : QString();
}

int SplitPdfPanel::interval() const {
  return m_intervalInput ? qMax(1, m_intervalInput->value()) : 1;
}

void SplitPdfPanel::onModeChanged(int index) {
  if (m_rangesInput) {
    m_rangesInput->setEnabled(index == 0);
  }
  if (m_intervalInput) {
    m_intervalInput->setEnabled(index == 1);
  }
}
