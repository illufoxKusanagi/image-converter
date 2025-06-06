#include "main_page.h"

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  m_sourceExtension = DropFileWidget::ImageExtension::JPG;
  setupImageLayout();
  if (m_targetExtension) {
    onImageTargetExtensionChanged();
  }
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
  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignBottom);
  attributeLayout->addWidget(m_targetExtension, 1, Qt::AlignBottom);
  mainLayout->addLayout(attributeLayout);
}

void MainPage::setupQualitySlider() {}

void MainPage::setupImageLayout() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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
  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    MessageBoxWidget messageBox("Error", "No file selected!",
                                MessageBoxWidget::Critical);
    messageBox.exec();
    return;
  }
  int quality = m_qualitySlider->getValue();
  if (sourcePaths.size() == 1) {
    m_dragWidget->convertImage(sourcePaths.first());
  } else {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory", QDir::homePath());
    if (outputDir.isEmpty()) {
      MessageBoxWidget messageBox("Error", "No output directory selected!",
                                  MessageBoxWidget::Critical);
      messageBox.exec();
      return;
    }
    int successCount = 0;
    int failureCount = 0;
    for (const QString &sourcePath : sourcePaths) {
      QImage image(sourcePath);
      if (image.isNull()) {
        failureCount++;
        continue;
      }
      QFileInfo fileInfo(sourcePath);
      QString baseOutputName = QDir(outputDir).filePath(fileInfo.baseName());
      if (m_dragWidget->saveImage(&image, baseOutputName, quality,
                                  &m_sourceExtension)) {
        successCount++;
      } else {
        failureCount++;
      }
    }
    MessageBoxWidget messageBox(
        "Batch Conversion Complete",
        QString("%1 image(s) converted successfully.\n%2 image(s) failed.")
            .arg(successCount)
            .arg(failureCount),
        MessageBoxWidget::Information);
    messageBox.exec();
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