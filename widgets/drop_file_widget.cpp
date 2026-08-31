#include "drop_file_widget.h"
#include <QFontMetrics>
#include <QScrollBar>

DropFileWidget::DropFileWidget(QWidget *parent, QString typeFile,
                               SliderWidget *sliderWidget,
                               ImageExtension *sourceExtension)
    : QGroupBox(parent), m_sourceExtension(sourceExtension),
      m_sliderWidget(sliderWidget), m_typeFile(typeFile),
      m_isDragHovered(false) {
  setObjectName("DropFileWidget");
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(12, 12, 12, 12);
  mainLayout->setSpacing(8);
  mainLayout->setAlignment(Qt::AlignCenter);

  updateDropZoneStyle();
  setupEmptyFileWidget();
  setupChosenFileWidget();
  setMinimumSize(420, 240);
  setMaximumHeight(260);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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

void DropFileWidget::updateDropZoneStyle() {
  QString borderColor = m_isDragHovered ? Colors::Primary500.name()
                                        : Colors::Secondary400.name();
  QString bgColor = m_isDragHovered ? Colors::Secondary50.name()
                                    : Colors::StandardWhite.name();
  setStyleSheet("QGroupBox#DropFileWidget {"
                "  border: 2px dashed " + borderColor + ";"
                "  border-radius: 12px;"
                "  background-color: " + bgColor + ";"
                "}"
                "QLabel {"
                "  background-color: transparent;"
                "}");
}

void DropFileWidget::setupEmptyFileWidget() {
  m_emptyFieldWidget = new QWidget(this);
  QVBoxLayout *emptyFieldLayout = new QVBoxLayout(m_emptyFieldWidget);
  emptyFieldLayout->setContentsMargins(0, 0, 0, 0);
  emptyFieldLayout->setSpacing(8);
  emptyFieldLayout->setAlignment(Qt::AlignCenter);

  m_icon = new QLabel(this);
  QLabel *label =
      new QLabel("Choose your " + m_typeFile + "(s) or drag here...", this);
  label->setStyleSheet("QLabel { color: " + Colors::Grey900.name() + "; " +
                       TextStyle::BodyMediumBold() + " }");

  QString iconRes = (m_typeFile == "PDF") ? ":/icons/icons/pdf.svg"
                                          : ":/icons/icons/upload.svg";
  QPixmap coloredIcon =
      createColoredIcon(iconRes, Colors::Primary600, 36, 36);
  m_browseButton = new ButtonAction(this, "Browse Files");
  m_browseButton->setFixedSize(140, 36);
  m_browseButton->setEnabled(true);
  m_icon->setPixmap(coloredIcon);
  m_icon->setAlignment(Qt::AlignCenter);
  label->setAlignment(Qt::AlignCenter);
  label->setWordWrap(true);

  emptyFieldLayout->addWidget(m_icon, 0, Qt::AlignCenter);
  emptyFieldLayout->addWidget(label, 0, Qt::AlignCenter);
  setupOrSeparatorLayout(emptyFieldLayout);
  emptyFieldLayout->addWidget(m_browseButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_emptyFieldWidget);
}

void DropFileWidget::setupChosenFileWidget() {
  m_chosenFileWidget = new QWidget(this);
  QVBoxLayout *chosenFileLayout = new QVBoxLayout(m_chosenFileWidget);
  chosenFileLayout->setContentsMargins(4, 4, 4, 4);
  chosenFileLayout->setSpacing(6);

  // Top header bar: Count/size label on left, "Clear All" on right
  QHBoxLayout *headerLayout = new QHBoxLayout();
  headerLayout->setContentsMargins(4, 0, 4, 0);
  m_chosenHeaderLabel = new QLabel("Selected Files", this);
  m_chosenHeaderLabel->setStyleSheet("QLabel { color: " + Colors::Grey900.name() +
                                     "; " + TextStyle::BodyMediumBold() + " }");

  m_clearAllButton = new QPushButton("Clear All", this);
  m_clearAllButton->setCursor(Qt::PointingHandCursor);
  m_clearAllButton->setStyleSheet(
      "QPushButton {"
      "  background: transparent;"
      "  border: none;"
      "  color: " + Colors::Danger500.name() + ";"
      "  " + TextStyle::SubtitleBigBold() +
      "}"
      "QPushButton:hover {"
      "  color: " + Colors::Danger600.name() + ";"
      "  text-decoration: underline;"
      "}");
  connect(m_clearAllButton, &QPushButton::clicked, this,
          &DropFileWidget::onClearAllPressed);

  headerLayout->addWidget(m_chosenHeaderLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
  headerLayout->addWidget(m_clearAllButton, 0, Qt::AlignRight | Qt::AlignVCenter);
  chosenFileLayout->addLayout(headerLayout);

  // Scroll Area for item cards
  m_fileScrollArea = new QScrollArea(this);
  m_fileScrollArea->setWidgetResizable(true);
  m_fileScrollArea->setFrameShape(QFrame::NoFrame);
  m_fileScrollArea->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }"
      "QScrollBar:vertical {"
      "  border: none;"
      "  background: " + Colors::Grey100.name() + ";"
      "  width: 6px;"
      "  border-radius: 3px;"
      "  margin: 0px 0px 0px 0px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: " + Colors::Grey400.name() + ";"
      "  min-height: 20px;"
      "  border-radius: 3px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "  background: " + Colors::Grey600.name() + ";"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "}");

  m_fileListContainer = new QWidget(m_fileScrollArea);
  m_fileListContainer->setStyleSheet("QWidget { background: transparent; }");
  m_fileListLayout = new QVBoxLayout(m_fileListContainer);
  m_fileListLayout->setContentsMargins(0, 0, 4, 0);
  m_fileListLayout->setSpacing(4);
  m_fileListLayout->setAlignment(Qt::AlignTop);
  m_fileScrollArea->setWidget(m_fileListContainer);
  chosenFileLayout->addWidget(m_fileScrollArea, 1);

  // Bottom action: "+ Add More Files"
  QHBoxLayout *footerLayout = new QHBoxLayout();
  footerLayout->setContentsMargins(0, 0, 0, 0);
  m_addMoreButton = new QPushButton("+ Add More Files", this);
  m_addMoreButton->setCursor(Qt::PointingHandCursor);
  m_addMoreButton->setStyleSheet(
      "QPushButton {"
      "  background-color: " + Colors::Grey100.name() + ";"
      "  border: 1px solid " + Colors::Grey300.name() + ";"
      "  border-radius: 6px;"
      "  padding: 4px 12px;"
      "  color: " + Colors::Grey800.name() + ";"
      "  " + TextStyle::SubtitleBigBold() +
      "}"
      "QPushButton:hover {"
      "  background-color: " + Colors::Grey200.name() + ";"
      "  border-color: " + Colors::Secondary400.name() + ";"
      "}");
  connect(m_addMoreButton, &QPushButton::clicked, this,
          &DropFileWidget::onBrowseButtonPressed);
  footerLayout->addStretch();
  footerLayout->addWidget(m_addMoreButton);
  footerLayout->addStretch();
  chosenFileLayout->addLayout(footerLayout);

  mainLayout->addWidget(m_chosenFileWidget);
  m_chosenFileWidget->setVisible(false);
}

void DropFileWidget::setupOrSeparatorLayout(QVBoxLayout *layout) {
  QString styling = "QLabel {"
                    "  color: " + Colors::Grey700.name() + ";"
                    "  " + TextStyle::SubtitleBigBold() +
                    "}";
  QString formatText = (m_typeFile == "PDF")
                           ? "pdf"
                           : "jpg, jpeg, png, webp, tiff, bmp, gif";
  QLabel *formatLabel = new QLabel(formatText, this);
  formatLabel->setStyleSheet(styling);
  formatLabel->setAlignment(Qt::AlignCenter);

  QWidget *orWidget = new QWidget(this);
  QHBoxLayout *orLayout = new QHBoxLayout(orWidget);
  orLayout->setAlignment(Qt::AlignCenter);
  orLayout->setSpacing(8);
  orLayout->setContentsMargins(40, 0, 40, 0);

  QLabel *optional = new QLabel("or", this);
  optional->setStyleSheet("QLabel {"
                          "  color: " + Colors::Grey700.name() + ";"
                          "  " + TextStyle::SubtitleMediumRegular() +
                          "}");
  optional->setAlignment(Qt::AlignCenter);

  QFrame *lineAbove = new QFrame(this);
  lineAbove->setFrameShape(QFrame::HLine);
  lineAbove->setFrameShadow(QFrame::Plain);
  lineAbove->setFixedHeight(1);
  lineAbove->setStyleSheet("background-color: " + Colors::Grey300.name() + ";");

  QFrame *lineBelow = new QFrame(this);
  lineBelow->setFrameShape(QFrame::HLine);
  lineBelow->setFrameShadow(QFrame::Plain);
  lineBelow->setFixedHeight(1);
  lineBelow->setStyleSheet("background-color: " + Colors::Grey300.name() + ";");

  orLayout->addWidget(lineAbove, 1);
  orLayout->addWidget(optional, 0);
  orLayout->addWidget(lineBelow, 1);

  layout->addWidget(formatLabel, 0, Qt::AlignCenter);
  layout->addWidget(orWidget);
}

void DropFileWidget::updateWidgetVisibility() {
  if (m_filePaths.isEmpty()) {
    m_emptyFieldWidget->setVisible(true);
    m_chosenFileWidget->setVisible(false);
  } else {
    m_emptyFieldWidget->setVisible(false);
    m_chosenFileWidget->setVisible(true);
    rebuildFileList();
  }
}

void DropFileWidget::rebuildFileList() {
  // Clear existing items in layout
  QLayoutItem *item;
  while ((item = m_fileListLayout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  qint64 totalBytes = 0;
  QString iconPath = (m_typeFile == "PDF") ? ":/icons/icons/pdf.svg"
                                          : ":/icons/icons/image.svg";
  QPixmap rowIcon = createColoredIcon(iconPath, Colors::Primary600, 18, 18);

  for (int i = 0; i < m_filePaths.size(); ++i) {
    const QString &path = m_filePaths.at(i);
    QFileInfo fi(path);
    qint64 size = fi.size();
    totalBytes += size;

    QWidget *card = new QWidget(m_fileListContainer);
    card->setFixedHeight(32);
    card->setStyleSheet(
        "QWidget {"
        "  background-color: " + Colors::Grey100.name() + ";"
        "  border: 1px solid " + Colors::Grey200.name() + ";"
        "  border-radius: 6px;"
        "}");

    QHBoxLayout *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(8, 2, 6, 2);
    cardLayout->setSpacing(6);

    QLabel *iconLabel = new QLabel(card);
    iconLabel->setPixmap(rowIcon);
    iconLabel->setFixedSize(18, 18);
    iconLabel->setStyleSheet("border: none; background: transparent;");

    QFontMetrics fm(font());
    QString elidedName = fm.elidedText(fi.fileName(), Qt::ElideMiddle, 210);
    QLabel *nameLabel = new QLabel(elidedName, card);
    nameLabel->setToolTip(fi.fileName());
    nameLabel->setStyleSheet("border: none; background: transparent; color: " +
                             Colors::Grey900.name() + "; " +
                             TextStyle::SubtitleBigBold());

    QLabel *sizeLabel = new QLabel(formatFileSize(size), card);
    sizeLabel->setStyleSheet("border: none; background: transparent; color: " +
                             Colors::Grey700.name() + "; " +
                             TextStyle::SubtitleMediumRegular());

    QPushButton *removeBtn = new QPushButton("✕", card);
    removeBtn->setFixedSize(18, 18);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setToolTip("Remove file");
    removeBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: " + Colors::Grey600.name() + ";"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  color: " + Colors::Danger500.name() + ";"
        "}");
    connect(removeBtn, &QPushButton::clicked, this,
            [this, i]() { removeFileAt(i); });

    cardLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);
    cardLayout->addWidget(nameLabel, 1, Qt::AlignVCenter);
    cardLayout->addWidget(sizeLabel, 0, Qt::AlignVCenter);
    cardLayout->addWidget(removeBtn, 0, Qt::AlignVCenter);

    m_fileListLayout->addWidget(card);
  }

  QString countStr = m_filePaths.size() == 1
                         ? "1 file selected"
                         : QString("%1 files selected").arg(m_filePaths.size());
  m_chosenHeaderLabel->setText(countStr + " • " + formatFileSize(totalBytes));
}

void DropFileWidget::removeFileAt(int index) {
  if (index >= 0 && index < m_filePaths.size()) {
    m_filePaths.removeAt(index);
    updateWidgetVisibility();
  }
}

void DropFileWidget::onClearAllPressed() {
  clearFiles();
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
    m_isDragHovered = true;
    updateDropZoneStyle();
    event->acceptProposedAction();
  }
}

void DropFileWidget::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void DropFileWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  m_isDragHovered = false;
  updateDropZoneStyle();
  event->accept();
}

void DropFileWidget::dropEvent(QDropEvent *event) {
  m_isDragHovered = false;
  updateDropZoneStyle();
  const QMimeData *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    QList<QUrl> urls = mimeData->urls();
    QStringList newPaths;
    for (const QUrl &url : urls) {
      if (url.isLocalFile()) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        QString ext = fileInfo.suffix().toLower();
        if (m_typeFile == "PDF") {
          if (ext == "pdf") {
            newPaths.append(
                QDir::cleanPath(QDir::fromNativeSeparators(filePath)));
          }
        } else {
          if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" ||
              ext == "tiff" || ext == "bmp" || ext == "gif") {
            newPaths.append(
                QDir::cleanPath(QDir::fromNativeSeparators(filePath)));
          }
        }
      }
    }
    if (!newPaths.isEmpty()) {
      for (const QString &p : newPaths) {
        if (!m_filePaths.contains(p)) {
          m_filePaths.append(p);
        }
      }
      updateWidgetVisibility();
      event->acceptProposedAction();
      return;
    }
  }
  event->ignore();
  updateWidgetVisibility();
}

void DropFileWidget::convertImage(const QString &sourcePath) {
  QImage image(sourcePath);
  if (image.isNull()) {
    MessageBoxWidget messageBox("Error",
                                QString("Failed to open image: %1")
                                    .arg(QFileInfo(sourcePath).fileName()),
                                MessageBoxWidget::Critical, this);
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
    return;
  }

  QFileInfo outFi(outputPathWithExt);
  QString outputPathWithoutExt = outFi.dir().filePath(outFi.completeBaseName());

  int quality = m_qualityValue;
  if (!saveImage(&image, outputPathWithoutExt, quality, m_sourceExtension)) {
    MessageBoxWidget messageBox(
        "Error", QString("Failed to save image as %1").arg(targetFormatString),
        MessageBoxWidget::Critical, this);
    messageBox.exec();
  } else {
    qint64 origSize = QFileInfo(sourcePath).size();
    qint64 outSize = QFileInfo(outputPathWithExt).size();
    QString sizeStats;
    if (origSize > 0 && outSize > 0) {
      double diffPct = (1.0 - (static_cast<double>(outSize) / origSize)) * 100.0;
      sizeStats = QString("\n\nOriginal: %1\nConverted: %2 (%3% %4)")
                      .arg(formatFileSize(origSize))
                      .arg(formatFileSize(outSize))
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
  }
}

void DropFileWidget::onBrowseButtonPressed() {
  QStringList fileNames;
  if (m_typeFile == "PDF") {
    fileNames =
        QFileDialog::getOpenFileNames(this, "Select PDF Files", QDir::homePath(),
                                      "PDF Files (*.pdf);;All Files (*)");
  } else {
    fileNames = QFileDialog::getOpenFileNames(
        this, "Select Image Files", QDir::homePath(),
        "Image Files (*.png *.jpg *.jpeg *.webp *.tiff *.bmp *.gif);;All Files (*)");
  }
  if (fileNames.isEmpty()) {
    return;
  }
  for (const QString &fn : fileNames) {
    QString clean = QDir::cleanPath(QDir::fromNativeSeparators(fn));
    if (!m_filePaths.contains(clean)) {
      m_filePaths.append(clean);
    }
  }
  updateWidgetVisibility();
}

QString DropFileWidget::getFilePath() const {
  return m_filePaths.isEmpty() ? QString() : m_filePaths.first();
}

QStringList DropFileWidget::getFilePaths() const { return m_filePaths; }

void DropFileWidget::clearFiles() {
  m_filePaths.clear();
  updateWidgetVisibility();
}

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
  case BMP:
    formatString = "BMP";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData());
    break;
  case GIF:
    formatString = "GIF";
    isSuccess = image->save(outputPath + "." + formatString.toLower(),
                            formatString.toLatin1().constData());
    break;
  case PDF:
    isSuccess = false;
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
    return "JPEG";
  case PNG:
    return "PNG";
  case WEBP:
    return "WEBP";
  case TIFF:
    return "TIFF";
  case BMP:
    return "BMP";
  case GIF:
    return "GIF";
  case PDF:
    return "PDF";
  default:
    Q_UNREACHABLE();
    return QString();
  }
}

QString DropFileWidget::formatFileSize(qint64 bytes) {
  if (bytes < 1024) {
    return QString("%1 B").arg(bytes);
  }
  double kb = bytes / 1024.0;
  if (kb < 1024.0) {
    return QString("%1 KB").arg(kb, 0, 'f', 1);
  }
  double mb = kb / 1024.0;
  if (mb < 1024.0) {
    return QString("%1 MB").arg(mb, 0, 'f', 2);
  }
  double gb = mb / 1024.0;
  return QString("%1 GB").arg(gb, 0, 'f', 2);
}
