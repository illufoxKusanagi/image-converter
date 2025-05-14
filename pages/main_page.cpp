#include "main_page.h"

/**
 * @brief Constructs the main page UI for image processing.
 *
 * Initializes the main layout with margins, spacing, and alignment, sets up the image-related interface components, and assigns the layout to the widget.
 */
MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(32, 32, 32, 32);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  setupImageLayout();
  setLayout(mainLayout);
}

/**
 * @brief Sets up the image input section with a quality slider and drag-and-drop widget.
 *
 * Creates a slider for adjusting image quality and a drag-and-drop widget for image file input, then adds the input widget to the image layout.
 */
void MainPage::setupImageInput() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_dragWidget = new DropFileWidget(this, "Image", m_qualitySlider);
  m_imageLayout->addWidget(m_dragWidget, 1, Qt::AlignCenter);
}

/**
 * @brief Sets up the extension selection widget for choosing source and target image formats.
 *
 * Creates a vertically arranged widget containing two dropdowns labeled "From" and "To" for selecting image file extensions.
 */
void MainPage::setupExtensionButton() {
  m_buttonWidget = new QWidget(this);
  m_buttonLayout = new QVBoxLayout(m_buttonWidget);
  m_buttonLayout->setContentsMargins(0, 0, 0, 0);
  m_buttonLayout->setSpacing(8);
  m_buttonLayout->setAlignment(Qt::AlignCenter);
  m_buttonWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  QStringList extensionOptions = {"jpg", "jpeg", "png", "webp", "tiff"};
  InputWidget *sourceExtension =
      new InputWidget(this, InputType("dropdown", "From"), extensionOptions);
  InputWidget *targetExtension =
      new InputWidget(this, InputType("dropdown", "To"), extensionOptions);
  m_buttonLayout->addWidget(sourceExtension);
  m_buttonLayout->addWidget(targetExtension);
}

/**
 * @brief Arranges the extension selection widget and quality slider horizontally within the image layout.
 *
 * Adds the extension selection widget and the image quality slider to a horizontal layout, aligning the slider to the top, and inserts this layout into the main image layout.
 */
void MainPage::setupImageAttribute() {
  QHBoxLayout *attributeLayout = new QHBoxLayout(this);
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  attributeLayout->addWidget(m_buttonWidget);
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  m_imageLayout->addLayout(attributeLayout);
}

/**
 * @brief Placeholder for quality slider setup logic.
 *
 * This method is currently unimplemented and reserved for future configuration of the image quality slider.
 */
void MainPage::setupQualitySlider() {}

/**
 * @brief Sets up the image-related layout and widgets for the main page.
 *
 * Arranges the image input, extension selection, quality control, and process button in a vertically stacked, centered layout within the main page.
 */
void MainPage::setupImageLayout() {
  m_imageLayout = new QVBoxLayout(this);
  m_imageLayout->setAlignment(Qt::AlignCenter);
  m_imageLayout->setSpacing(8);
  ButtonAction *processButton = new ButtonAction(this, "Process Image", "no");
  processButton->setEnabled(true);
  processButton->setSize(256, 42);
  connect(processButton, &QPushButton::clicked, this,
          MainPage::onProcessButtonClicked);
  setupImageInput();
  setupExtensionButton();
  // setupQualitySlider();
  setupImageAttribute();
  mainLayout->addLayout(m_imageLayout);
  mainLayout->addWidget(processButton);
}

/**
 * @brief Handles the "Process Image" button click by converting the selected image.
 *
 * Retrieves the file path from the drop file widget and initiates image conversion using that path.
 */
void MainPage::onProcessButtonClicked() {
  QString filePath = m_dragWidget->getFilePath();
  m_dragWidget->convertImage(filePath);
}