#include "main_page.h"

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(32);

  // ButtonAction *browseButton = new ButtonAction("Pilih Gambar", "upload",
  // this); browseButton->setFixedSize(150, 50); browseButton->setEnabled(true);
  setupImageLayout();
  // mainLayout->addWidget(browseButton);
  setLayout(mainLayout);
  // setStyleSheet("QWidget {"
  //               "    background-color: " +
  //               Colors::Secondary600.name() +
  //               ";"
  //               "}");
}

void MainPage::setupImageInput() {
  DropFileWidget *dragWidget = new DropFileWidget(this, "Image");
  // dragWidget->setStyleSheet("background-color: " +
  // Colors::Secondary100.name() +
  //                           ";"
  //                           "border: 2px dashed " +
  //                           Colors::Secondary900.name() + ";");
  m_imageLayout->addWidget(dragWidget);
}

void MainPage::setupExtensionButton() {
  QWidget *buttonWidget = new QWidget(this);
  QVBoxLayout *buttonLayout = new QVBoxLayout(buttonWidget);
  buttonLayout->setContentsMargins(0, 0, 0, 0);
  buttonLayout->setSpacing(16);
  buttonWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  InputWidget *sourceExtension =
      new InputWidget(InputType("dropdown", "From"), this);
  InputWidget *targetExtension =
      new InputWidget(InputType("dropdown", "To"), this);
  buttonLayout->addWidget(sourceExtension);
  buttonLayout->addWidget(targetExtension);
  m_imageLayout->addWidget(buttonWidget);
}

void MainPage::setupQualitySlider() {
  SliderWidget *qualitySlider = new SliderWidget(this, "Image Quality");
  m_imageLayout->addWidget(qualitySlider);
}

void MainPage::setupImageLayout() {
  m_imageLayout = new QVBoxLayout(this);
  setupImageInput();
  setupExtensionButton();
  setupQualitySlider();
  mainLayout->addLayout(m_imageLayout);
}