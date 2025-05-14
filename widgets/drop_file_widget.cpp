#include "drop_file_widget.h"

DropFileWidget::DropFileWidget(QWidget *parent, QString typeFile,
                               SliderWidget *sliderWidget,
                               ImageExtension *sourceExtension)
    : QGroupBox(parent), m_sourceExtension(sourceExtension),
      m_sliderWidget(sliderWidget) {
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

  QLabel *icon = new QLabel(this);
  QLabel *label =
      new QLabel("Chose your file or drag your " + typeFile + " here...", this);
  QPixmap pixmap(":/icons/icons/upload.svg");
  QPixmap coloredIcon =
      createColoredIcon(":/icons/icons/upload.svg", Colors::Primary600, 40, 40);
  QPixmap scaledPixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio);
  ButtonAction *browseButton = new ButtonAction(this, "Browse");
  browseButton->setFixedSize(128, 40);
  browseButton->setEnabled(true);
  icon->setPixmap(coloredIcon);
  icon->setAlignment(Qt::AlignCenter);
  label->setAlignment(Qt::AlignCenter);
  label->setMinimumHeight(50);
  label->setMaximumWidth(400);
  label->setWordWrap(true);
  label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  mainLayout->addWidget(icon, 0, Qt::AlignCenter);
  mainLayout->addWidget(label, 0, Qt::AlignCenter);
  setupOrSeparatorLayout();
  mainLayout->addWidget(browseButton, 0, Qt::AlignCenter);
  setFixedSize(320, 320);
  setLayout(mainLayout);
  connect(m_sliderWidget, &SliderWidget::valueChanged, this,
          &DropFileWidget::onSliderValueChanged);
  connect(browseButton, &ButtonAction::clicked, this,
          &DropFileWidget::onBrowseButtonPressed);
  setAcceptDrops(true);
}

void DropFileWidget::setupOrSeparatorLayout() {
  QString styling = "QLabel{"
                    "color: " +
                    Colors::Grey400.name() + ";" + TextStyle::BodySmallBold() +
                    "}";
  QWidget *orWidget = new QWidget(this);
  QHBoxLayout *orLayout = new QHBoxLayout(orWidget);
  QLabel *formatLabel = new QLabel("jpg, jpeg, png, webp", this);
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
  mainLayout->addWidget(formatLabel, 0, Qt::AlignCenter);
  mainLayout->addWidget(orWidget);
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
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty()) {
    return;
  }
  m_sourcePath = urls.first().toLocalFile();
  if (m_sourcePath.isEmpty()) {
    return;
  }
}

void DropFileWidget::convertImage(const QString sourcePath) {
  QImage image(sourcePath);
  if (image.isNull()) {
    QMessageBox::warning(this, "Error", "Failed to open image!");
    return;
  }
  QString outputPath =
      QFileDialog::getSaveFileName(this, "Save Image", QDir::homePath());
  int lastDotIndex = outputPath.lastIndexOf(".");
  if (lastDotIndex != -1) {
    outputPath = outputPath.left(lastDotIndex);
  }

  int quality = m_qualityValue;
  if (!saveImage(&image, outputPath, quality, m_sourceExtension)) {
    QMessageBox::warning(this, "Error", "Failed to save image as JPG!");
    return;
  }

  QMessageBox::information(
      this, "Success",
      QString("Image converted successfully at " + outputPath));
}

void DropFileWidget::onBrowseButtonPressed() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Select a file", QDir::homePath(),
      "Images (*.png *.jpg *.jpeg *.webp);;All Files (*)");
  if (fileName.isEmpty()) {
    return;
  }
  m_sourcePath = fileName;
}

QString DropFileWidget::getFilePath() { return m_sourcePath; }

void DropFileWidget::onSliderValueChanged() {
  m_qualityValue = m_sliderWidget->getValue();
}

// void DropFileWidget::setSourceExtension(const ImageExtension
// &sourceExtension) {
//   switch (sourceExtension) {
//   case JPG:
//     m_sourceExtension = "JPG";
//     break;
//   case JPEG:
//     m_sourceExtension = "JPEG";
//     break;
//   case PNG:
//     m_sourceExtension = "PNG";
//     break;
//   case WEBP:
//     m_sourceExtension = "WEBP";
//     break;
//   case TIFF:
//     m_sourceExtension = "TIFF";
//     break;
//   case PDF:
//     m_sourceExtension = "PDF";
//     break;
//   }
// }

bool DropFileWidget::saveImage(const QImage *image, const QString &outputPath,
                               const int quality,
                               const ImageExtension *sourceExtension) {
  bool isSuccess = false;
  QString formatString;
  switch (*sourceExtension) {
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
  }
  return isSuccess;
}
