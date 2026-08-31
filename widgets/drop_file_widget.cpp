#include "drop_file_widget.h"
#include "ui-kit/components/badge.h"
#include "ui-kit/components/separator.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
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
  connect(m_browseButton, &QPushButton::clicked, this,
          &DropFileWidget::onBrowseButtonPressed);
  setAcceptDrops(true);
  updateWidgetVisibility();

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &DropFileWidget::applyThemeStyles);
  applyThemeStyles();
}

void DropFileWidget::updateDropZoneStyle() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  QString borderColor = m_isDragHovered ? ui::StyleHelper::toHexString(c.primary)
                                        : ui::StyleHelper::toHexString(c.border);
  QString bgColor = m_isDragHovered ? ui::StyleHelper::toHexString(c.accent)
                                    : ui::StyleHelper::toHexString(c.card);
  setStyleSheet(QString("QGroupBox#DropFileWidget {"
                        "  border: 2px dashed %1;"
                        "  border-radius: %2px;"
                        "  background-color: %3;"
                        "}"
                        "QLabel {"
                        "  background-color: transparent;"
                        "}")
                    .arg(borderColor)
                    .arg(r.lg)
                    .arg(bgColor));
}

void DropFileWidget::setupEmptyFileWidget() {
  m_emptyFieldWidget = new QWidget(this);
  QVBoxLayout *emptyFieldLayout = new QVBoxLayout(m_emptyFieldWidget);
  emptyFieldLayout->setContentsMargins(0, 0, 0, 0);
  emptyFieldLayout->setSpacing(8);
  emptyFieldLayout->setAlignment(Qt::AlignCenter);

  m_icon = new QLabel(this);
  m_emptyLabel = new QLabel("Choose your " + m_typeFile + "(s) or drag here...", this);

  m_browseButton = new ui::Button("Browse Files", ui::ButtonVariant::Outline, ui::ButtonSize::Default, this);
  m_browseButton->setMinimumWidth(140);

  m_icon->setAlignment(Qt::AlignCenter);
  m_emptyLabel->setAlignment(Qt::AlignCenter);
  m_emptyLabel->setWordWrap(true);

  emptyFieldLayout->addWidget(m_icon, 0, Qt::AlignCenter);
  emptyFieldLayout->addWidget(m_emptyLabel, 0, Qt::AlignCenter);
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

  m_clearAllButton = new QPushButton("Clear All", this);
  m_clearAllButton->setCursor(Qt::PointingHandCursor);
  connect(m_clearAllButton, &QPushButton::clicked, this,
          &DropFileWidget::onClearAllPressed);

  headerLayout->addWidget(m_chosenHeaderLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
  headerLayout->addWidget(m_clearAllButton, 0, Qt::AlignRight | Qt::AlignVCenter);
  chosenFileLayout->addLayout(headerLayout);

  // Scroll Area for item cards
  m_fileScrollArea = new QScrollArea(this);
  m_fileScrollArea->setWidgetResizable(true);
  m_fileScrollArea->setFrameShape(QFrame::NoFrame);

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
  m_addMoreButton = new ui::Button("+ Add More Files", ui::ButtonVariant::Ghost, ui::ButtonSize::Small, this);
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
  m_formatLabel = new QLabel(
      m_typeFile == "PDF"
          ? "Supports .pdf documents"
          : "Supports .png, .jpg, .jpeg, .webp, .tiff, .bmp, .gif",
      this);
  m_formatLabel->setAlignment(Qt::AlignCenter);

  QWidget *orWidget = new QWidget(this);
  QHBoxLayout *orLayout = new QHBoxLayout(orWidget);
  orLayout->setAlignment(Qt::AlignCenter);
  orLayout->setSpacing(8);
  orLayout->setContentsMargins(40, 0, 40, 0);

  m_orLabel = new QLabel("or", this);
  m_orLabel->setAlignment(Qt::AlignCenter);

  ui::Separator *lineAbove = new ui::Separator(Qt::Horizontal, this);
  ui::Separator *lineBelow = new ui::Separator(Qt::Horizontal, this);

  orLayout->addWidget(lineAbove, 1);
  orLayout->addWidget(m_orLabel, 0);
  orLayout->addWidget(lineBelow, 1);

  layout->addWidget(m_formatLabel, 0, Qt::AlignCenter);
  layout->addWidget(orWidget);
}

void DropFileWidget::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  updateDropZoneStyle();

  if (m_emptyLabel) {
    m_emptyLabel->setFont(t.font(t.sizeBase, QFont::DemiBold));
    m_emptyLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                    .arg(ui::StyleHelper::toHexString(c.foreground)));
  }

  if (m_formatLabel) {
    m_formatLabel->setFont(t.font(t.sizeXs, QFont::Normal));
    m_formatLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                     .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_orLabel) {
    m_orLabel->setFont(t.font(t.sizeXs, QFont::Normal));
    m_orLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_chosenHeaderLabel) {
    m_chosenHeaderLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
    m_chosenHeaderLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                           .arg(ui::StyleHelper::toHexString(c.foreground)));
  }

  if (m_clearAllButton) {
    m_clearAllButton->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_clearAllButton->setStyleSheet(QString(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: %1;"
        "}"
        "QPushButton:hover {"
        "  text-decoration: underline;"
        "}")
        .arg(ui::StyleHelper::toHexString(c.destructive)));
  }

  if (m_fileScrollArea) {
    m_fileScrollArea->setStyleSheet(QString(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: transparent;"
        "  width: 6px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %1;"
        "  min-height: 20px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}")
        .arg(ui::StyleHelper::toHexString(c.border)));
  }

  if (m_browseButton) {
    m_browseButton->setIcon(ui::Icon::get(":/icons/icons/lucide-folder-open.svg", c.foreground, QSize(16, 16)));
  }

  if (m_addMoreButton) {
    m_addMoreButton->setIcon(ui::Icon::get(":/icons/icons/lucide-plus.svg", c.foreground, QSize(14, 14)));
  }

  QString iconRes = (m_typeFile == "PDF") ? ":/icons/icons/lucide-file-text.svg"
                                          : ":/icons/icons/lucide-upload-cloud.svg";
  QPixmap coloredIcon = ui::Icon::pixmap(iconRes, c.primary, 48, 48);
  if (m_icon) {
    m_icon->setPixmap(coloredIcon);
  }

  rebuildFileList();
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
  if (!m_fileListLayout) return;

  // Clear existing items in layout
  QLayoutItem *item;
  while ((item = m_fileListLayout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  qint64 totalBytes = 0;
  QString iconPath = (m_typeFile == "PDF") ? ":/icons/icons/lucide-file-text.svg"
                                          : ":/icons/icons/lucide-image.svg";
  QPixmap rowIcon = ui::Icon::pixmap(iconPath, c.primary, 18, 18);

  for (int i = 0; i < m_filePaths.size(); ++i) {
    const QString &path = m_filePaths.at(i);
    QFileInfo fi(path);
    qint64 size = fi.size();
    totalBytes += size;

    QWidget *card = new QWidget(m_fileListContainer);
    card->setFixedHeight(32);
    card->setStyleSheet(QString(
        "QWidget {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "}")
        .arg(ui::StyleHelper::toHexString(c.secondary))
        .arg(ui::StyleHelper::toHexString(c.border))
        .arg(r.sm));

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
    nameLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
    nameLabel->setStyleSheet(QString("border: none; background: transparent; color: %1;")
                                 .arg(ui::StyleHelper::toHexString(c.foreground)));

    ui::Badge *sizeBadge =
        new ui::Badge(formatFileSize(size), ui::BadgeVariant::Secondary, card);

    QPushButton *upBtn = new QPushButton(card);
    upBtn->setFixedSize(20, 20);
    upBtn->setCursor(Qt::PointingHandCursor);
    upBtn->setToolTip("Move up");
    upBtn->setEnabled(i > 0);
    upBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-chevron-up.svg", i > 0 ? c.mutedForeground : c.border, QSize(14, 14)));
    upBtn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(upBtn, &QPushButton::clicked, this, [this, i]() { moveFileUp(i); });

    QPushButton *downBtn = new QPushButton(card);
    downBtn->setFixedSize(20, 20);
    downBtn->setCursor(Qt::PointingHandCursor);
    downBtn->setToolTip("Move down");
    downBtn->setEnabled(i < m_filePaths.size() - 1);
    downBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-chevron-down.svg", i < m_filePaths.size() - 1 ? c.mutedForeground : c.border, QSize(14, 14)));
    downBtn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(downBtn, &QPushButton::clicked, this, [this, i]() { moveFileDown(i); });

    QPushButton *removeBtn = new QPushButton(card);
    removeBtn->setFixedSize(20, 20);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setToolTip("Remove file");
    removeBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-x.svg", c.mutedForeground, QSize(14, 14)));
    removeBtn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(removeBtn, &QPushButton::clicked, this, [this, i]() { removeFileAt(i); });

    cardLayout->addWidget(iconLabel);
    cardLayout->addWidget(nameLabel, 1);
    cardLayout->addWidget(sizeBadge);
    cardLayout->addWidget(upBtn);
    cardLayout->addWidget(downBtn);
    cardLayout->addWidget(removeBtn);

    m_fileListLayout->addWidget(card);
  }

  QString countStr = m_filePaths.size() == 1
                         ? "1 file selected"
                         : QString("%1 files selected").arg(m_filePaths.size());
  if (m_chosenHeaderLabel) {
    m_chosenHeaderLabel->setText(countStr + " • " + formatFileSize(totalBytes));
  }
}

void DropFileWidget::removeFileAt(int index) {
  if (index >= 0 && index < m_filePaths.size()) {
    m_filePaths.removeAt(index);
    updateWidgetVisibility();
  }
}

void DropFileWidget::moveFileUp(int index) {
  if (index > 0 && index < m_filePaths.size()) {
    m_filePaths.swapItemsAt(index, index - 1);
    rebuildFileList();
  }
}

void DropFileWidget::moveFileDown(int index) {
  if (index >= 0 && index < m_filePaths.size() - 1) {
    m_filePaths.swapItemsAt(index, index + 1);
    rebuildFileList();
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
            newPaths.append(QDir::cleanPath(filePath));
          }
        } else {
          QStringList validExts = {"png", "jpg", "jpeg", "webp", "tiff", "bmp", "gif"};
          if (validExts.contains(ext)) {
            newPaths.append(QDir::cleanPath(filePath));
          }
        }
      }
    }
    for (const QString &np : newPaths) {
      if (!m_filePaths.contains(np)) {
        m_filePaths.append(np);
      }
    }
    updateWidgetVisibility();
  }
}

QString DropFileWidget::getFilePath() const {
  return m_filePaths.isEmpty() ? QString() : m_filePaths.first();
}

QStringList DropFileWidget::getFilePaths() const {
  return m_filePaths;
}

void DropFileWidget::clearFiles() {
  m_filePaths.clear();
  updateWidgetVisibility();
}

QString DropFileWidget::imageExtensionToString(
    const ImageExtension &sourceExtension) const {
  switch (sourceExtension) {
  case JPG:
    return "jpg";
  case JPEG:
    return "jpeg";
  case PNG:
    return "png";
  case WEBP:
    return "webp";
  case TIFF:
    return "tiff";
  case BMP:
    return "bmp";
  case GIF:
    return "gif";
  case PDF:
    return "pdf";
  default:
    return "";
  }
}

QString DropFileWidget::formatFileSize(qint64 bytes) {
  if (bytes < 1024) {
    return QString("%1 B").arg(bytes);
  } else if (bytes < 1024 * 1024) {
    return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
  } else {
    return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
  }
}

bool DropFileWidget::saveImage(const QImage *image, const QString &outputPath,
                              const int quality,
                              const ImageExtension *sourceExtension) {
  if (!image || image->isNull() || !sourceExtension) {
    return false;
  }
  QString format = imageExtensionToString(*sourceExtension);
  int effectiveQuality = quality;
  if (*sourceExtension == PNG) {
    effectiveQuality = -1;
  }
  return image->save(outputPath, format.toUpper().toUtf8().constData(),
                     effectiveQuality);
}

void DropFileWidget::convertImage(const QString &sourcePath) {
  Q_UNUSED(sourcePath);
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

void DropFileWidget::onSliderValueChanged() {
  if (m_sliderWidget) {
    m_qualityValue = m_sliderWidget->getValue();
  }
}
