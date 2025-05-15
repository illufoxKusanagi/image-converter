#include "main_page.h"

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(32, 32, 32, 32);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  setupImageLayout();
  setLayout(mainLayout);
}

void MainPage::setupImageInput() {}

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"jpg", "jpeg", "png", "webp", "tiff"};
  m_targetExtension =
      new InputWidget(this, InputType("dropdown", "Target"), extensionOptions);
  connect(m_targetExtension, &InputWidget::valueChanged, this,
          MainPage::onImageTargetExtensionChanged);
  m_targetExtension->getValue();
}

void MainPage::setupImageAttribute() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignBottom);
  attributeLayout->addWidget(m_targetExtension, 1, Qt::AlignBottom);
  // attributeLayout->setAlignment(Qt::AlignTop);
  mainLayout->addLayout(attributeLayout);
}

void MainPage::setupQualitySlider() {}

void MainPage::setupImageLayout() {
  m_dragWidget =
      new DropFileWidget(this, "Image", m_qualitySlider, &m_sourceExtension);
  ButtonAction *processButton = new ButtonAction(this, "Process Image", "no");
  processButton->setEnabled(true);
  processButton->setSize(256, 42);
  connect(processButton, &QPushButton::clicked, this,
          MainPage::onProcessButtonClicked);
  mainLayout->addWidget(m_dragWidget);
  setupImageAttribute();
  mainLayout->addWidget(processButton, 0, Qt::AlignCenter);
}

void MainPage::onProcessButtonClicked() {
  QString filePath = m_dragWidget->getFilePath();
  if (!filePath.isEmpty())
    m_dragWidget->convertImage(filePath);
  else {
    MessageBoxWidget messageBox("Error", "No file selected!",
                                MessageBoxWidget::Critical);
    messageBox.exec();
    return;
  }
}

void MainPage::onImageTargetExtensionChanged() {
  double value = m_targetExtension->getValue();
  if (value == 0) {
    m_sourceExtension = DropFileWidget::ImageExtension::JPG;
  } else if (value == 1) {
    m_sourceExtension = DropFileWidget::ImageExtension::JPEG;
  } else if (value == 2) {
    m_sourceExtension = DropFileWidget::ImageExtension::PNG;
  } else if (value == 3) {
    m_sourceExtension = DropFileWidget::ImageExtension::WEBP;
  } else if (value == 4) {
    m_sourceExtension = DropFileWidget::ImageExtension::TIFF;
  }
}