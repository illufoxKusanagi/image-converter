#include "main_page.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)),
      m_targetImageExtension(DropFileWidget::ImageExtension::JPG) {
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  if (m_targetExtension) {
    onImageTargetExtensionChanged();
  }
  setLayout(mainLayout);
}

MainPage::~MainPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"jpg", "jpeg", "png", "webp", "tiff", "bmp", "gif"};
  m_targetExtension =
      new InputWidget(this, InputType("dropdown", "Target"), extensionOptions);
  connect(m_targetExtension, &InputWidget::valueChanged, this,
          &MainPage::onImageTargetExtensionChanged);
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

void MainPage::setupImageLayout() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_dragWidget =
      new DropFileWidget(this, "Image", m_qualitySlider, &m_targetImageExtension);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setFixedHeight(20);
  m_progressBar->setAlignment(Qt::AlignCenter);
  m_progressBar->setTextVisible(true);
  m_progressBar->setVisible(false);
  m_progressBar->setStyleSheet(
      "QProgressBar {"
      "  border: 1px solid " + Colors::Grey300.name() + ";"
      "  border-radius: 8px;"
      "  text-align: center;"
      "  background-color: " + Colors::Grey100.name() + ";"
      "  color: " + Colors::StandardBlack.name() + ";"
      "  " + TextStyle::BodySmallBold() +
      "}"
      "QProgressBar::chunk {"
      "  background-color: " + Colors::Primary500.name() + ";"
      "  border-radius: 7px;"
      "}");

  m_processButton = new ButtonAction(this, "Process Image", "no");
  m_processButton->setEnabled(true);
  m_processButton->setSize(320, 42);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton = new ButtonAction(this, "Cancel", "no");
  m_cancelButton->setSize(320, 42);
  m_cancelButton->setVisible(false);
  m_cancelButton->setStyleSheet(
      "QPushButton {"
      "    padding: 12px 4px;"
      "    border: 0 solid;"
      "    border-radius: 12px;"
      "    background-color: " + Colors::Danger500.name() + ";"
      "    color: " + Colors::StandardWhite.name() + ";"
      "    " + TextStyle::BodyMediumBold() +
      "}"
      "QPushButton:hover {"
      "    background-color: " + Colors::Danger600.name() + ";"
      "}"
      "QPushButton:pressed {"
      "    background-color: " + Colors::Danger700.name() + ";"
      "}");
  connect(m_cancelButton, &QPushButton::clicked, this,
          &MainPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  setupImageAttribute();
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
}

void MainPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  m_progressBar->setVisible(isProcessing);
  m_targetExtension->setEnabled(!isProcessing);
  m_dragWidget->setEnabled(!isProcessing);

  if (isProcessing) {
    m_qualitySlider->setEnabled(false);
  } else {
    onImageTargetExtensionChanged();
  }
}

void MainPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Cancelling...");
  }
}

void MainPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    MessageBoxWidget messageBox("Error", "No file selected!",
                                MessageBoxWidget::Critical, this);
    messageBox.exec();
    return;
  }

  int quality = m_qualitySlider->getValue();
  DropFileWidget::ImageExtension targetExt = m_targetImageExtension;
  QString targetFormatString = m_dragWidget->imageExtensionToString(targetExt);

  if (sourcePaths.size() == 1) {
    QString sourcePath = sourcePaths.first();
    QString filter = targetFormatString.toUpper() + " (*." +
                     targetFormatString.toLower() + ")";
    QString suggestedName =
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_converted");

    QString outputPathWithExt = QFileDialog::getSaveFileName(
        this, "Save Image As", suggestedName, filter);

    if (outputPathWithExt.isEmpty()) {
      return;
    }

    QString outputPathWithoutExt = outputPathWithExt;
    int lastDotIndex = outputPathWithExt.lastIndexOf('.');
    if (lastDotIndex != -1) {
      outputPathWithoutExt = outputPathWithExt.left(lastDotIndex);
    }

    setProcessingState(true);
    m_isCancelled = false;
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Converting image... %p%");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run([this, sourcePath, outputPathWithoutExt,
                                              outputPathWithExt, quality, targetExt,
                                              targetFormatString, dragWidget]() {
      QImage image(sourcePath);
      bool success = false;
      if (!image.isNull() && !m_isCancelled) {
        success = dragWidget->saveImage(&image, outputPathWithoutExt, quality, &targetExt);
      }

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, success, outputPathWithExt, targetFormatString]() {
            m_progressBar->setValue(1);
            setProcessingState(false);
            m_cancelButton->setText("Cancel");

            if (m_isCancelled) {
              MessageBoxWidget messageBox(
                  "Cancelled", "Image conversion was cancelled.",
                  MessageBoxWidget::Information, this);
              messageBox.exec();
            } else if (success) {
              qint64 origSize = QFileInfo(sourcePath).size();
              qint64 outSize = QFileInfo(outputPathWithExt).size();
              QString sizeStats;
              if (origSize > 0 && outSize > 0) {
                double diffPct = (1.0 - (static_cast<double>(outSize) / origSize)) * 100.0;
                sizeStats = QString("\n\nOriginal: %1\nConverted: %2 (%3% %4)")
                                .arg(DropFileWidget::formatFileSize(origSize))
                                .arg(DropFileWidget::formatFileSize(outSize))
                                .arg(qAbs(diffPct), 0, 'f', 1)
                                .arg(diffPct >= 0 ? "smaller" : "larger");
              }

              MessageBoxWidget messageBox(
                  "Success",
                  QString("Image converted successfully to %1%2")
                      .arg(outputPathWithExt)
                      .arg(sizeStats),
                  MessageBoxWidget::Information, this);
              messageBox.exec();
            } else {
              MessageBoxWidget messageBox(
                  "Error",
                  QString("Failed to save image as %1").arg(targetFormatString),
                  MessageBoxWidget::Critical, this);
              messageBox.exec();
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
  } else {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0);
    m_progressBar->setFormat(QString("Converting %v of %m images (%p%)"));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir, quality,
                                              targetExt, totalFiles, dragWidget]() {
      int successCount = 0;
      int failureCount = 0;
      qint64 totalOrigBytes = 0;
      qint64 totalOutputBytes = 0;
      QSet<QString> usedBaseNames;
      QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

      for (int i = 0; i < totalFiles; ++i) {
        if (m_isCancelled) {
          break;
        }

        const QString &sourcePath = sourcePaths.at(i);
        QImage image(sourcePath);
        if (image.isNull()) {
          failureCount++;
        } else {
          QFileInfo fileInfo(sourcePath);
          QString baseName = fileInfo.baseName();
          QString uniqueBaseName = baseName;
          int suffix = 1;
          while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
                 QFile::exists(QDir(outputDir).filePath(uniqueBaseName + "." + ext))) {
            uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
          }
          usedBaseNames.insert(uniqueBaseName.toLower());

          QString baseOutputName = QDir(outputDir).filePath(uniqueBaseName);
          if (dragWidget->saveImage(&image, baseOutputName, quality, &targetExt)) {
            successCount++;
            totalOrigBytes += fileInfo.size();
            QString outputFilePath = baseOutputName + "." + ext;
            totalOutputBytes += QFileInfo(outputFilePath).size();
          } else {
            failureCount++;
          }
        }

        QMetaObject::invokeMethod(
            this,
            [this, i]() {
              m_progressBar->setValue(i + 1);
            },
            Qt::QueuedConnection);
      }

      QMetaObject::invokeMethod(
          this,
          [this, successCount, failureCount, totalOrigBytes, totalOutputBytes]() {
            setProcessingState(false);
            m_cancelButton->setText("Cancel");

            QString title = m_isCancelled ? "Batch Conversion Cancelled"
                                          : "Batch Conversion Complete";
            QString message = QString("%1 image(s) converted successfully.\n%2 image(s) failed.")
                                  .arg(successCount)
                                  .arg(failureCount);

            if (totalOrigBytes > 0 && totalOutputBytes > 0 && successCount > 0) {
              double diffPct =
                  (1.0 - (static_cast<double>(totalOutputBytes) / totalOrigBytes)) * 100.0;
              message += QString("\n\nTotal Original: %1\nTotal Converted: %2 (%3% %4)")
                             .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                             .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                             .arg(qAbs(diffPct), 0, 'f', 1)
                             .arg(diffPct >= 0 ? "smaller" : "larger");
            }

            if (m_isCancelled) {
              message += "\n(Operation was stopped early by user)";
            }

            MessageBoxWidget messageBox(title, message,
                                        MessageBoxWidget::Information, this);
            messageBox.exec();
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
  }
}

void MainPage::onImageTargetExtensionChanged() {
  double value = m_targetExtension->getValue();
  if (value == 0) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 1) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPEG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 2) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PNG;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("PNG (Lossless - Quality N/A)");
  } else if (value == 3) {
    m_targetImageExtension = DropFileWidget::ImageExtension::WEBP;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 4) {
    m_targetImageExtension = DropFileWidget::ImageExtension::TIFF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 5) {
    m_targetImageExtension = DropFileWidget::ImageExtension::BMP;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("BMP (Uncompressed)");
  } else if (value == 6) {
    m_targetImageExtension = DropFileWidget::ImageExtension::GIF;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("GIF (Indexed Color)");
  }
}