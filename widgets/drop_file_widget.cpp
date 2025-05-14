#include "drop_file_widget.h"

/**
 * @brief Constructs a DropFileWidget for selecting or dragging image files.
 *
 * Initializes the widget with an icon, descriptive label, "Browse" button, and a separator indicating supported image formats. Integrates with a slider widget to adjust image quality and enables drag-and-drop functionality for file selection.
 *
 * @param typeFile The type of file to be described in the UI prompt (e.g., "image").
 */
DropFileWidget::DropFileWidget(QWidget *parent, QString typeFile,
                               SliderWidget *sliderWidget)
    : QGroupBox(parent), m_sliderWidget(sliderWidget) {
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
  connect(browseButton, &ButtonAction::clicked, this, onBrowseButtonPressed);
  setAcceptDrops(true);
}

/**
 * @brief Adds a separator layout with supported image formats and an "or" divider to the widget.
 *
 * Inserts a label listing supported image formats above a horizontal layout containing two lines separated by the word "or", visually distinguishing alternative file selection methods.
 */
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

/**
 * @brief Loads an icon from the specified path, scales it, and applies a color overlay.
 *
 * Creates a pixmap from the given icon file, scales it to the specified width and height, and overlays it with the provided color using source-in composition. Returns an empty pixmap if the icon cannot be loaded.
 *
 * @param iconPath Path to the icon file.
 * @param color Color to apply as an overlay.
 * @param width Desired width of the resulting pixmap.
 * @param height Desired height of the resulting pixmap.
 * @return QPixmap The colorized and scaled pixmap, or an empty pixmap if loading fails.
 */
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

/**
 * @brief Handles drag enter events to accept files dragged into the widget.
 *
 * Accepts the drag event if the dragged data contains URLs, enabling file drop functionality.
 *
 * @param event The drag enter event containing information about the dragged data.
 */
void DropFileWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

/**
 * @brief Handles file drop events by extracting and storing the local file path from the dropped data.
 *
 * If the dropped data contains URLs, the first local file path is stored internally. If no valid file is found, the event is ignored.
 */
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

/**
 * @brief Converts an image file to JPG format with the current quality setting.
 *
 * Loads the image from the specified source path, converts it to JPG using the quality value set by the slider, and saves it with a ".jpg" extension. Displays a message box indicating success or failure.
 *
 * @param sourcePath Path to the source image file to be converted.
 */
void DropFileWidget::convertImage(const QString sourcePath) {
  QImage image(sourcePath);
  if (image.isNull()) {
    QMessageBox::warning(this, "Error", "Failed to open image!");
    return;
  }

  QString outputPath = sourcePath;
  int lastDotIndex = outputPath.lastIndexOf(".");
  if (lastDotIndex != -1) {
    outputPath = outputPath.left(lastDotIndex);
  }
  outputPath += ".jpg";

  int quality = m_qualityValue;
  if (!image.save(outputPath, "JPG", quality)) {
    QMessageBox::warning(this, "Error", "Failed to save image as JPG!");
    return;
  }

  QMessageBox::information(
      this, "Success",
      QString("Image converted successfully at " + outputPath));
}

/**
 * @brief Opens a file dialog for the user to select an image file and stores the selected file path.
 *
 * If the user selects a file, its path is saved internally; if the dialog is canceled, no action is taken.
 */
void DropFileWidget::onBrowseButtonPressed() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Select a file", QDir::homePath(),
      "Images (*.png *.jpg *.jpeg *.webp);;All Files (*)");
  if (fileName.isEmpty()) {
    return;
  }
  m_sourcePath = fileName;
}

/**
 * @brief Returns the currently selected image file path.
 *
 * @return QString The path of the selected source file, or an empty string if no file is selected.
 */
QString DropFileWidget::getFilePath() { return m_sourcePath; }

/**
 * @brief Updates the internal image quality value based on the slider's current position.
 *
 * Synchronizes the widget's quality setting with the value selected in the associated slider.
 */
void DropFileWidget::onSliderValueChanged() {
  m_qualityValue = m_sliderWidget->getValue();
}