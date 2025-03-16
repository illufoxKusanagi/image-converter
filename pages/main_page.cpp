#include "main_page.h"

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(32);

  ButtonAction *browseButton = new ButtonAction("Pilih Gambar", "upload", this);
  browseButton->setFixedSize(150, 50);
  browseButton->setEnabled(true);
  setupImageLayout();
  mainLayout->addWidget(browseButton);
  setLayout(mainLayout);
  // setStyleSheet("QWidget {"
  //               "    background-color: " +
  //               Colors::Secondary600.name() +
  //               ";"
  //               "}");
}

void MainPage::setupImageInput() {
  QWidget *dragWidget = new QWidget(this);
  QVBoxLayout *dragLayout = new QVBoxLayout(dragWidget);
  dragLayout->setContentsMargins(0, 0, 0, 0);
  dragWidget->setStyleSheet("background-color: " + Colors::Secondary100.name() +
                            ";"
                            "border: 2px dashed " +
                            Colors::Secondary900.name() + ";");
  QLabel *dragIcon = new QLabel(this);
  QLabel *dragLabel = new QLabel("Drop your image here...", this);
  QPixmap pixmap(":/icons/icons/upload.svg");
  QPixmap scaledPixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio);
  // dragIcon->setFixedSize(80, 80);
  dragIcon->setPixmap(scaledPixmap);
  dragIcon->setAlignment(Qt::AlignCenter);
  dragLabel->setAlignment(Qt::AlignCenter);
  dragLayout->addWidget(dragIcon);
  dragLayout->addWidget(dragLabel);
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

void MainPage::setupImageLayout() {
  m_imageLayout = new QHBoxLayout(this);
  setupImageInput();
  setupExtensionButton();
  mainLayout->addLayout(m_imageLayout);
}