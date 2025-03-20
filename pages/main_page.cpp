#include "main_page.h"

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(32, 32, 32, 32);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  setupImageLayout();
  setLayout(mainLayout);
}

void MainPage::setupImageInput() {
  DropFileWidget *dragWidget = new DropFileWidget(this, "Image");
  m_imageLayout->addWidget(dragWidget, 1, Qt::AlignCenter);
}

void MainPage::setupExtensionButton() {
  QWidget *buttonWidget = new QWidget(this);
  QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  buttonLayout->setSpacing(16);
  buttonLayout->setAlignment(Qt::AlignCenter);
  buttonWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  QStringList extensionOptions = {"jpg", "jpeg", "png", "webp", "tiff"};
  InputWidget *sourceExtension =
      new InputWidget(this, InputType("dropdown", "From"), extensionOptions);
  InputWidget *targetExtension =
      new InputWidget(this, InputType("dropdown", "To"), extensionOptions);
  buttonLayout->addWidget(sourceExtension);
  buttonLayout->addWidget(targetExtension);
  m_imageLayout->addWidget(buttonWidget);
}

void MainPage::setupQualitySlider() {
  SliderWidget *qualitySlider = new SliderWidget(this, "Image Quality");
  m_imageLayout->addWidget(qualitySlider, 1, Qt::AlignCenter);
}

void MainPage::setupImageLayout() {
  m_imageLayout = new QVBoxLayout(this);
  m_imageLayout->setAlignment(Qt::AlignCenter);
  m_imageLayout->setSpacing(8);
  setupImageInput();
  setupExtensionButton();
  setupQualitySlider();
  mainLayout->addLayout(m_imageLayout);
}