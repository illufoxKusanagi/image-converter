#include "drop_file_widget.h"

DropFileWidget::DropFileWidget(QWidget *parent, QString typeFile,
                               SliderWidget *sliderWidget,
                               ImageExtension *sourceExtension)
    : QGroupBox(parent), m_sourceExtension(sourceExtension),
      m_sliderWidget(sliderWidget), m_typeFile(typeFile) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignCenter);
  setStyleSheet("QGroupBox {"
                "border: 2px solid " +
                Colors::Secondary400.name() +
                ";"
                "border-radius: 12px;"
                "}"
                "QLabel {"
                " background-color: transparent; " +
                TextStyle::BodyBigBold() + "color : " + Colors::Grey900.name() +
                ";"
                "}"
                "QFrame[frameShape=\"4\"] {"
                "    background-color: " +
                Colors::Grey400.name() +
                ";"
                "    color: transparent;"
                "}");
  setupEmptyFileWidget();
  setupChosenFileWidget();
  setFixedSize(320, 320);
  setLayout(mainLayout);
  if (m_sliderWidget) {
    connect(m_sliderWidget, &SliderWidget::valueChanged, this,
            &DropFileWidget::onSliderValueChanged);
  } else {
    m_qualityValue = 100;
  }
  connect(m_browseButton, &ButtonAction::clicked, this,
          &DropFileWidget::onBrowseButtonPressed);
  setAcceptDrops(true);
  updateWidgetVisibility();
}

void DropFileWidget::setupEmptyFileWidget() {
  m_emptyFieldWidget = new QWidget(this);
  QVBoxLayout *emptyFieldLayout = new QVBoxLayout(m_emptyFieldWidget);
  m_emptyFieldWidget->setFixedSize(256, 256);
  m_icon = new QLabel(this);
  QLabel *label = new QLabel(
      "Chose your file(s) or drag your " + m_typeFile + " here...", this);
  QPixmap pixmap(":/icons/icons/upload.svg");
  QPixmap coloredIcon =
      createColoredIcon(":/icons/icons/upload.svg", Colors::Primary600, 40, 40);
  QPixmap scaledPixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio);
  m_browseButton = new ButtonAction(this, "Browse");
  m_browseButton->setFixedSize(128, 40);
  m_browseButton->setEnabled(true);
  m_icon->setPixmap(coloredIcon);
  m_icon->setAlignment(Qt::AlignCenter);
  label->setAlignment(Qt::AlignCenter);
  label->setMinimumHeight(50);
  label->setMaximumWidth(400);
  label->setWordWrap(true);
  label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  emptyFieldLayout->addWidget(m_icon, 0, Qt::AlignCenter);
  emptyFieldLayout->addWidget(label, 0, Qt::AlignCenter);
  setupOrSeparatorLayout(emptyFieldLayout);
  emptyFieldLayout->addWidget(m_browseButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_emptyFieldWidget);
}

void DropFileWidget::setupChosenFileWidget() {
  m_chosenFileWidget = new QWidget(this);
  QVBoxLayout *chosenFileLayout = new QVBoxLayout(m_chosenFileWidget);
  m_chosenIcon = new QLabel(this);
  QPixmap coloredIcon =
      createColoredIcon(":/icons/icons/image.svg", Colors::Primary600, 40, 40);
  m_chosenIcon->setPixmap(coloredIcon);
  m_chosenIcon->setAlignment(Qt::AlignCenter);
  m_chosenLabel = new QLabel("Selected files: " + m_sourcePath, this);
  m_chosenLabel->setAlignment(Qt::AlignCenter);
  m_chosenLabel->setMinimumHeight(50);
  m_chosenLabel->setMaximumWidth(400);
  m_chosenLabel->setWordWrap(true);
  m_chosenLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  chosenFileLayout->addWidget(m_chosenIcon, 0, Qt::AlignCenter);
  chosenFileLayout->addWidget(m_chosenLabel, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_chosenFileWidget);
  m_chosenFileWidget->setVisible(false);
}

void DropFileWidget::setupOrSeparatorLayout(QVBoxLayout *layout) {
  QString styling = "QLabel{"
                    "color: " +
                    Colors::Grey400.name() + ";" + TextStyle::BodySmallBold() +
                    "}";
  QWidget *orWidget = new QWidget(this);
  QHBoxLayout *orLayout = new QHBoxLayout(orWidget);
  QLabel *formatLabel = new QLabel("jpg, jpeg, png, webp, tiff", this);
  QLabel *optional = new QLabel("or", this);
  QFrame *lineAbove = new QFrame(this);
  QFrame *lineBelow = new QFrame(this);
  orLayout->setAlignment(Qt::AlignCenter);
  orLayout->setSpacing(8);
  lineAbove->setFrameShape(QFrame::HLine);
  lineAbove->setFrameShadow(QFrame::Plain);
  lineAbove->setFixedHeight(1);
  lineBelow->setFrameShape(QFrame::HLine);
  lineBelow->setFrameShadow(QFrame::Plain);
  lineBelow->setFixedHeight(1);
  formatLabel->setStyleSheet(styling);
  optional->setStyleSheet(styling);
  orLayout->addWidget(lineAbove, 1);
  orLayout->addWidget(optional, 0);
  orLayout->addWidget(lineBelow, 1);
  orLayout->setContentsMargins(40, 0, 40, 0);
  optional->setAlignment(Qt::AlignCenter);
  layout->addWidget(formatLabel, 0, Qt::AlignCenter);
  layout->addWidget(orWidget);
}

void DropFileWidget::updateWidgetVisibility() {
  if (m_sourcePath.isEmpty()) {
    m_emptyFieldWidget->setVisible(true);
    m_chosenFileWidget->setVisible(false);
  } else {
    m_emptyFieldWidget->setVisible(false);
    m_chosenFileWidget->setVisible(true);
    if (m_filePaths.size() == 1) {
      m_chosenLabel->setText("Selected file: " +
                             QFileInfo(m_filePaths.first()).fileName());
    } else {
      m_chosenLabel->setText(
          QString("%1 files selected").arg(m_filePaths.size()));
    }
  }
}

QPixmap DropFileWidget::createColoredIcon(const QString &iconPath,
                                          const QColor &color, int width,
                                          int height) {
  QPixmap originalPixmap(iconPath);
  if (originalPixmap.isNull()) {
    return QPixmap();
  }
  originalPixmap = originalPixmap.scaled(width, height, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
  QPixmap coloredPixmap(originalPixmap.size());
  coloredPixmap.fill(Qt::transparent);
  QPainter painter(&coloredPixmap);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.drawPixmap(0, 0, originalPixmap);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(coloredPixmap.rect(), color);
  painter.end();

  return coloredPixmap;
}

void DropFileWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void DropFileWidget::dropEvent(QDropEvent *event) {
  const QMimeData *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    QList<QUrl> urls = mimeData->urls();
    QStringList newPaths;
    for (const QUrl &url : urls) {
      if (url.isLocalFile()) {
        QString filePath = url.toLocalFile();
        // Basic check for image extensions, can be more robust
        QFileInfo fileInfo(filePath);
        QString ext = fileInfo.suffix().toLower();
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" ||
            ext == "tiff") {
          newPaths.append(
              QDir::cleanPath(QDir::fromNativeSeparators(filePath)));
        }
      }
    }
    if (!newPaths.isEmpty()) {
      m_filePaths = newPaths;
      updateWidgetVisibility();
      event->acceptProposedAction();
      return;
    }
  }
  event->ignore();
  updateWidgetVisibility();
}

// void DropFileWidget::convertImage(const QString sourcePath) {
//   QImage image(sourcePath);
//   if (image.isNull()) {
//     MessageBoxWidget messageBox("Error", QString("Failed to open image!"),
//                                 MessageBoxWidget::Critical);
//     messageBox.exec();
//     return;
//   }
//   QString outputPath =
//       QFileDialog::getSaveFileName(this, "Save Image", QDir::homePath());
//   if (outputPath.isEmpty()) {
//     MessageBoxWidget messageBox("Error", "No image selected!",
//                                 MessageBoxWidget::Warning);
//     messageBox.exec();
//     return;
//   }
//   int lastDotIndex = outputPath.lastIndexOf(".");
//   if (lastDotIndex != -1) {
//     outputPath = outputPath.left(lastDotIndex);
//   }

//   int quality = m_qualityValue;
//   if (!saveImage(&image, outputPath, quality, m_sourceExtension)) {
//     MessageBoxWidget messageBox(
//         "Error", QString("Failed to save image as " + *m_sourceExtension),
//         MessageBoxWidget::Critical);
//     messageBox.exec();
//     return;
//   }
//   MessageBoxWidget messageBox(
//       "Success", QString("Image converted successfully at " + outputPath),
//       MessageBoxWidget::Information);
//   messageBox.exec();
// }

void DropFileWidget::convertImage(const QString sourcePath) {
  QImage image(sourcePath);
  if (image.isNull()) {
    MessageBoxWidget messageBox("Error",
                                QString("Failed to open image: %1")
                                    .arg(QFileInfo(sourcePath).fileName()),
                                MessageBoxWidget::Critical);
    messageBox.exec();
    return;
  }

  QString targetFormatString = imageExtensionToString(*m_sourceExtension);
  QString filter = targetFormatString.toUpper() + " (*." +
                   targetFormatString.toLower() + ")";
  QString suggestedName =
      QFileInfo(sourcePath)
          .dir()
          .filePath(QFileInfo(sourcePath).baseName() + "_converted");

  QString outputPathWithExt = QFileDialog::getSaveFileName(
      this, "Save Image As", suggestedName, filter);

  if (outputPathWithExt.isEmpty()) {
    // User cancelled
    return;
  }

  // Extract base path without extension, as saveImage appends it
  QString outputPathWithoutExt = outputPathWithExt;
  int lastDotIndex = outputPathWithExt.lastIndexOf('.');
  if (lastDotIndex != -1) {
    // Check if the extension matches the target format, if not, strip it.
    // Or, always strip it and let saveImage handle it. For simplicity, let's
    // strip.
    outputPathWithoutExt = outputPathWithExt.left(lastDotIndex);
  }

  int quality = m_qualityValue; // This is m_sliderWidget->getValue() or default
  if (!saveImage(&image, outputPathWithoutExt, quality, m_sourceExtension)) {
    MessageBoxWidget messageBox(
        "Error", QString("Failed to save image as %1").arg(targetFormatString),
        MessageBoxWidget::Critical);
    messageBox.exec();
  } else {
    MessageBoxWidget messageBox(
        "Success",
        QString("Image converted successfully to %1").arg(outputPathWithExt),
        MessageBoxWidget::Information);
    messageBox.exec();
  }
}

void DropFileWidget::onBrowseButtonPressed() {
  QStringList fileNames = QFileDialog::getOpenFileNames(
      this, "Select a file", QDir::homePath(),
      "Images (*.png *.jpg *.jpeg *.webp *.tiff);;All Files (*)");
  if (fileNames.isEmpty()) {
    MessageBoxWidget messageBox("Error", "No image selected!",
                                MessageBoxWidget::Warning);
    messageBox.exec();
    return;
  }
  m_filePaths = fileNames;
  updateWidgetVisibility();
}

QString DropFileWidget::getFilePath() { return m_sourcePath; }
QStringList DropFileWidget::getFilePaths() { return m_filePaths; }

void DropFileWidget::onSliderValueChanged() {
  m_qualityValue = m_sliderWidget->getValue();
}

bool DropFileWidget::saveImage(const QImage *image, const QString &outputPath,
                               const int quality,
                               const ImageExtension *extensionType) {
  bool isSuccess = false;
  QString formatString;
  switch (*extensionType) {
  case JPG:
    formatString = "JPG";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData(), quality);
    break;
  case JPEG:
    formatString = "JPEG";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData(), quality);
    break;
  case PNG:
    formatString = "PNG";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData(), -1);
    break;
  case WEBP:
    formatString = "WEBP";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData(), quality);
    break;
  case TIFF:
    formatString = "TIFF";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData(), quality);
    break;
  default:
    Q_UNREACHABLE();
  }
  return isSuccess;
}

QString
DropFileWidget::imageExtensionToString(const ImageExtension &extension) const {
  switch (extension) {
  case JPG:
    return "JPG";
  case JPEG:
    return "JPEG"; // Or "JPG" if Qt treats them identically for saving
  case PNG:
    return "PNG";
  case WEBP:
    return "WEBP";
  case TIFF:
    return "TIFF";
  default:
    return QString();
  }
}