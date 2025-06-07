#include "pdf_page.h"

PdfPage::PdfPage(QWidget *parent) : QWidget(parent) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignHCenter);
  DropFileWidget *dragWidget = new DropFileWidget(this, "PDF");
  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  ButtonAction *processButton = new ButtonAction(this, "Compress PDF", "no");
  processButton->setEnabled(true);
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  mainLayout->addWidget(dragWidget);
  mainLayout->addWidget(m_qualitySlider);
  mainLayout->addWidget(processButton);
  connect(processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);
  setLayout(mainLayout);
}

void PdfPage::onProcessButtonClicked() {}
