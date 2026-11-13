#include "pages/pdf_page.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPdfDocument>
#include <QTransform>

PdfPage::PdfPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(12);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  m_toolTabs = new ui::Tabs(ui::TabVariant::Segmented, this);
  m_toolTabs->addTab("Compress");
  m_toolTabs->addTab("Merge");
  m_toolTabs->addTab("Split");
  m_toolTabs->addTab("Organize");
  m_toolTabs->addTab("PDF to Images");
  m_toolTabs->addTab("Images to PDF");
  m_toolTabs->addTab("Grayscale");
  m_toolTabs->addTab("Markdown / HTML");
  connect(m_toolTabs, &ui::Tabs::currentChanged, this,
          &PdfPage::onToolTabChanged);

  m_dragWidget = new DropFileWidget(this, "PDF");
  m_imageDragWidget = new DropFileWidget(this, "IMAGE");
  m_imageDragWidget->setVisible(false);
  m_docDragWidget = new DropFileWidget(this, "Markdown / HTML");
  m_docDragWidget->setVisible(false);

  m_qualityRowWidget = new QWidget(this);
  QHBoxLayout *attributeLayout = new QHBoxLayout(m_qualityRowWidget);
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  m_qualitySlider = new SliderWidget(m_qualityRowWidget, "PDF Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setValue(50);

  QStringList presetOptions = {"Custom", "Web / Email", "Balanced",
                               "High Quality (preserve images)"};
  m_presetSelect = new ui::Select(presetOptions, m_qualityRowWidget);
  m_presetSelect->setCurrentIndex(2); // Default to Balanced (50)
  m_presetField = new ui::FormField("Compression Preset", m_presetSelect,
                                    m_qualityRowWidget);
  m_presetField->setToolTip(
      "Text and vector graphics are retained. Below quality 80, images may "
      "lose detail. At quality 80 or above, images are not downsampled or "
      "newly JPEG-encoded. PDF rewriting can alter forms and annotations "
      "and invalidates digital signatures. Keep the original PDF.");

  connect(m_presetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfPage::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfPage::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_presetField, 1, Qt::AlignTop);

  m_optionsStack = new QStackedWidget(this);
  setupToolPanels();

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignCenter);
  m_progressLabel->setVisible(false);

  m_progressBar = new ui::Progress(0, this);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Compress PDF", ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);

  m_cancelButton =
      new ui::Button("Cancel Compression", ui::ButtonVariant::Destructive,
                     ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &PdfPage::onCancelButtonClicked);

  mainLayout->addWidget(m_toolTabs);
  mainLayout->addWidget(m_dragWidget);
  mainLayout->addWidget(m_imageDragWidget);
  mainLayout->addWidget(m_docDragWidget);
  mainLayout->addWidget(m_optionsStack);
  mainLayout->addWidget(m_qualityRowWidget);
  mainLayout->addWidget(m_progressLabel);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
  setLayout(mainLayout);

  initControllers();
  updateToolModeUi();

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &PdfPage::applyThemeStyles);
  applyThemeStyles();
}

PdfPage::~PdfPage() {
  for (PdfToolController *ctrl : {
           static_cast<PdfToolController *>(m_compressController),
           static_cast<PdfToolController *>(m_mergeController),
           static_cast<PdfToolController *>(m_splitController),
           static_cast<PdfToolController *>(m_organizeController),
           static_cast<PdfToolController *>(m_pdfToImagesController),
           static_cast<PdfToolController *>(m_imagesToPdfController),
           static_cast<PdfToolController *>(m_grayscaleController),
           static_cast<PdfToolController *>(m_markdownController)}) {
    if (ctrl && ctrl->isRunning()) {
      ctrl->cancelAndWait();
    }
  }
}

void PdfPage::initControllers() {
  m_compressController = new CompressController(this);
  m_mergeController = new MergeController(this);
  m_splitController = new SplitController(this);
  m_organizeController = new OrganizeController(this);
  m_pdfToImagesController = new PdfToImagesController(this);
  m_imagesToPdfController = new ImagesToPdfController(this);
  m_grayscaleController = new GrayscaleController(this);
  m_markdownController = new MarkdownPdfController(this);

  for (PdfToolController *ctrl : {
           static_cast<PdfToolController *>(m_compressController),
           static_cast<PdfToolController *>(m_mergeController),
           static_cast<PdfToolController *>(m_splitController),
           static_cast<PdfToolController *>(m_organizeController),
           static_cast<PdfToolController *>(m_pdfToImagesController),
           static_cast<PdfToolController *>(m_imagesToPdfController),
           static_cast<PdfToolController *>(m_grayscaleController),
           static_cast<PdfToolController *>(m_markdownController)}) {
    connect(ctrl, &PdfToolController::progressChanged, this,
            &PdfPage::onControllerProgress, Qt::QueuedConnection);
    connect(ctrl, &PdfToolController::fileStatusChanged, this,
            &PdfPage::onControllerFileStatus, Qt::QueuedConnection);
  }

  connect(m_compressController, &CompressController::finished, this,
          &PdfPage::onCompressFinished, Qt::QueuedConnection);
  connect(m_mergeController, &MergeController::finished, this,
          &PdfPage::onMergeFinished, Qt::QueuedConnection);
  connect(m_splitController, &SplitController::finished, this,
          &PdfPage::onSplitFinished, Qt::QueuedConnection);
  connect(m_organizeController, &OrganizeController::finished, this,
          &PdfPage::onOrganizeFinished, Qt::QueuedConnection);
  connect(m_pdfToImagesController, &PdfToImagesController::finished, this,
          &PdfPage::onPdfToImagesFinished, Qt::QueuedConnection);
  connect(m_imagesToPdfController, &ImagesToPdfController::finished, this,
          &PdfPage::onImagesToPdfFinished, Qt::QueuedConnection);
  connect(m_grayscaleController, &GrayscaleController::finished, this,
          &PdfPage::onGrayscaleFinished, Qt::QueuedConnection);
  connect(m_markdownController, &MarkdownPdfController::finished, this,
          &PdfPage::onMarkdownPdfFinished, Qt::QueuedConnection);
}

DropFileWidget *PdfPage::activeDropWidget() const {
  if (m_currentTool == PdfToolMode::ImagesToPdf) {
    return m_imageDragWidget;
  }
  if (m_currentTool == PdfToolMode::MarkdownToPdf) {
    return m_docDragWidget;
  }
  return m_dragWidget;
}

void PdfPage::setupToolPanels() {
  // Page 0: Compress (uses m_qualityRowWidget)
  QWidget *compressDummy = new QWidget(m_optionsStack);
  m_optionsStack->addWidget(compressDummy);

  // Page 1: Merge PDF
  m_mergePanel = new MergePdfPanel(m_optionsStack);
  connect(m_mergePanel, &MergePdfPanel::syncRequested, this,
          &PdfPage::syncMergeListFromDropWidget);
  m_optionsStack->addWidget(m_mergePanel);

  // Page 2: Split PDF
  m_splitPanel = new SplitPdfPanel(m_optionsStack);
  m_optionsStack->addWidget(m_splitPanel);

  // Page 3: Organize / Rotate / Delete PDF Pages
  m_organizePanel = new QWidget(m_optionsStack);
  QVBoxLayout *organizeLayout = new QVBoxLayout(m_organizePanel);
  organizeLayout->setContentsMargins(0, 0, 0, 0);
  organizeLayout->setSpacing(6);

  QHBoxLayout *orgToolbarLayout = new QHBoxLayout();
  orgToolbarLayout->setContentsMargins(0, 0, 0, 0);
  orgToolbarLayout->setSpacing(6);

  m_organizeHintLabel =
      new QLabel("Pages (drag or use buttons to reorder, rotate, or delete):",
                 m_organizePanel);
  m_orgLoadBtn =
      new ui::Button("Load Pages from PDF", ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Small, m_organizePanel);
  m_orgRotLeftBtn =
      new ui::Button("Rotate Left (-90°)", ui::ButtonVariant::Outline,
                     ui::ButtonSize::Small, m_organizePanel);
  m_orgRotRightBtn =
      new ui::Button("Rotate Right (+90°)", ui::ButtonVariant::Outline,
                     ui::ButtonSize::Small, m_organizePanel);
  m_orgRot180Btn = new ui::Button("Rotate 180°", ui::ButtonVariant::Outline,
                                  ui::ButtonSize::Small, m_organizePanel);
  m_orgMoveUpBtn = new ui::Button("Move Up", ui::ButtonVariant::Outline,
                                  ui::ButtonSize::Small, m_organizePanel);
  m_orgMoveDownBtn = new ui::Button("Move Down", ui::ButtonVariant::Outline,
                                    ui::ButtonSize::Small, m_organizePanel);
  m_orgDeleteBtn = new ui::Button("Delete Page", ui::ButtonVariant::Destructive,
                                  ui::ButtonSize::Small, m_organizePanel);
  m_orgResetBtn = new ui::Button("Reset", ui::ButtonVariant::Ghost,
                                 ui::ButtonSize::Small, m_organizePanel);

  connect(m_orgLoadBtn, &QPushButton::clicked, this,
          [this]() { loadOrganizePagesFromSelectedPdf(false); });
  connect(m_orgRotLeftBtn, &QPushButton::clicked, this,
          [this]() { rotateSelectedOrganizePages(-90); });
  connect(m_orgRotRightBtn, &QPushButton::clicked, this,
          [this]() { rotateSelectedOrganizePages(90); });
  connect(m_orgRot180Btn, &QPushButton::clicked, this,
          [this]() { rotateSelectedOrganizePages(180); });
  connect(m_orgMoveUpBtn, &QPushButton::clicked, this,
          [this]() { moveSelectedOrganizePage(-1); });
  connect(m_orgMoveDownBtn, &QPushButton::clicked, this,
          [this]() { moveSelectedOrganizePage(1); });
  connect(m_orgDeleteBtn, &QPushButton::clicked, this,
          &PdfPage::deleteSelectedOrganizePages);
  connect(m_orgResetBtn, &QPushButton::clicked, this,
          &PdfPage::resetOrganizePages);

  orgToolbarLayout->addWidget(m_orgLoadBtn);
  orgToolbarLayout->addWidget(m_orgRotLeftBtn);
  orgToolbarLayout->addWidget(m_orgRotRightBtn);
  orgToolbarLayout->addWidget(m_orgRot180Btn);
  orgToolbarLayout->addWidget(m_orgMoveUpBtn);
  orgToolbarLayout->addWidget(m_orgMoveDownBtn);
  orgToolbarLayout->addWidget(m_orgDeleteBtn);
  orgToolbarLayout->addWidget(m_orgResetBtn);
  orgToolbarLayout->addStretch(1);

  m_organizePageList = new QListWidget(m_organizePanel);
  m_organizePageList->setViewMode(QListWidget::ListMode);
  m_organizePageList->setIconSize(QSize(40, 56));
  m_organizePageList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_organizePageList->setDragDropMode(QAbstractItemView::InternalMove);
  m_organizePageList->setDefaultDropAction(Qt::MoveAction);
  m_organizePageList->setFixedHeight(140);

  organizeLayout->addWidget(m_organizeHintLabel);
  organizeLayout->addLayout(orgToolbarLayout);
  organizeLayout->addWidget(m_organizePageList);
  m_optionsStack->addWidget(m_organizePanel);

  // Page 4: PDF to Images
  m_pdfToImagesPanel = new QWidget(m_optionsStack);
  QHBoxLayout *exportLayout = new QHBoxLayout(m_pdfToImagesPanel);
  exportLayout->setContentsMargins(0, 0, 0, 0);
  exportLayout->setSpacing(12);

  QStringList exportFormats = {"PNG", "JPG", "WEBP", "TIFF"};
  m_exportImageFormatSelect = new ui::Select(exportFormats, m_pdfToImagesPanel);
  m_exportImageFormatField = new ui::FormField(
      "Image Format", m_exportImageFormatSelect, m_pdfToImagesPanel);
  connect(m_exportImageFormatSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &PdfPage::onExportFormatChanged);

  QStringList dpiOptions = {"72 DPI (Screen)", "150 DPI (Medium)",
                            "300 DPI (Print)", "600 DPI (High Detail)"};
  m_exportDpiSelect = new ui::Select(dpiOptions, m_pdfToImagesPanel);
  m_exportDpiSelect->setCurrentIndex(2); // Default 300 DPI
  m_exportDpiField = new ui::FormField("Render Resolution", m_exportDpiSelect,
                                       m_pdfToImagesPanel);

  m_exportPageRangeInput =
      new ui::Input("All pages, or e.g. 1-3, 5", m_pdfToImagesPanel);
  m_exportPageRangeField = new ui::FormField(
      "Page Filter (optional)", m_exportPageRangeInput, m_pdfToImagesPanel);

  exportLayout->addWidget(m_exportImageFormatField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_exportDpiField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_exportPageRangeField, 2, Qt::AlignTop);
  m_optionsStack->addWidget(m_pdfToImagesPanel);

  // Page 5: Images to Merged PDF
  m_imagesToPdfPanel = new QWidget(m_optionsStack);
  QVBoxLayout *imgPdfRootLayout = new QVBoxLayout(m_imagesToPdfPanel);
  imgPdfRootLayout->setContentsMargins(0, 0, 0, 0);
  imgPdfRootLayout->setSpacing(8);

  QHBoxLayout *imgOrderHeaderLayout = new QHBoxLayout();
  imgOrderHeaderLayout->setContentsMargins(0, 0, 0, 0);
  imgOrderHeaderLayout->setSpacing(8);

  m_imageOrderHintLabel =
      new QLabel("Image Page Order (drag items or use buttons to reorder):",
                 m_imagesToPdfPanel);
  m_imgSyncBtn =
      new ui::Button("Sync from Drop Zone", ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Small, m_imagesToPdfPanel);
  m_imgUpBtn = new ui::Button("Move Up", ui::ButtonVariant::Outline,
                              ui::ButtonSize::Small, m_imagesToPdfPanel);
  m_imgDownBtn = new ui::Button("Move Down", ui::ButtonVariant::Outline,
                                ui::ButtonSize::Small, m_imagesToPdfPanel);

  connect(m_imgSyncBtn, &QPushButton::clicked, this,
          &PdfPage::syncImageListFromDropWidget);
  connect(m_imgUpBtn, &QPushButton::clicked, this, &PdfPage::moveImageItemUp);
  connect(m_imgDownBtn, &QPushButton::clicked, this,
          &PdfPage::moveImageItemDown);

  imgOrderHeaderLayout->addWidget(m_imageOrderHintLabel, 1);
  imgOrderHeaderLayout->addWidget(m_imgSyncBtn);
  imgOrderHeaderLayout->addWidget(m_imgUpBtn);
  imgOrderHeaderLayout->addWidget(m_imgDownBtn);

  m_imageOrderList = new QListWidget(m_imagesToPdfPanel);
  m_imageOrderList->setDragDropMode(QAbstractItemView::InternalMove);
  m_imageOrderList->setDefaultDropAction(Qt::MoveAction);
  m_imageOrderList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_imageOrderList->setFixedHeight(96);

  QHBoxLayout *imgPdfOptionsRow = new QHBoxLayout();
  imgPdfOptionsRow->setContentsMargins(0, 0, 0, 0);
  imgPdfOptionsRow->setSpacing(12);

  QStringList pageSizes = {"Fit to Image", "A4 (210 × 297 mm)",
                           "Letter (8.5 × 11 in)"};
  m_imgPdfPageSizeSelect = new ui::Select(pageSizes, m_imagesToPdfPanel);
  m_imgPdfPageSizeField = new ui::FormField("Page Size", m_imgPdfPageSizeSelect,
                                            m_imagesToPdfPanel);

  QStringList orientations = {"Auto (Match Image)", "Portrait", "Landscape"};
  m_imgPdfOrientationSelect = new ui::Select(orientations, m_imagesToPdfPanel);
  m_imgPdfOrientationField = new ui::FormField(
      "Orientation", m_imgPdfOrientationSelect, m_imagesToPdfPanel);

  QStringList margins = {"No Margin (0 pt)", "Small (18 pt)", "Medium (36 pt)",
                         "Large (54 pt)"};
  m_imgPdfMarginSelect = new ui::Select(margins, m_imagesToPdfPanel);
  m_imgPdfMarginField = new ui::FormField("Page Margin", m_imgPdfMarginSelect,
                                          m_imagesToPdfPanel);

  imgPdfOptionsRow->addWidget(m_imgPdfPageSizeField, 1, Qt::AlignTop);
  imgPdfOptionsRow->addWidget(m_imgPdfOrientationField, 1, Qt::AlignTop);
  imgPdfOptionsRow->addWidget(m_imgPdfMarginField, 1, Qt::AlignTop);

  imgPdfRootLayout->addLayout(imgOrderHeaderLayout);
  imgPdfRootLayout->addWidget(m_imageOrderList);
  imgPdfRootLayout->addLayout(imgPdfOptionsRow);
  m_optionsStack->addWidget(m_imagesToPdfPanel);

  // Page 6: Grayscale (uses m_qualityRowWidget)
  QWidget *grayscaleDummy = new QWidget(m_optionsStack);
  m_optionsStack->addWidget(grayscaleDummy);

  // Page 7: Markdown / HTML to PDF
  m_markdownPanel = new QWidget(m_optionsStack);
  QHBoxLayout *mdLayout = new QHBoxLayout(m_markdownPanel);
  mdLayout->setContentsMargins(0, 0, 0, 0);
  mdLayout->setSpacing(12);

  QStringList mdPageSizes = {"A4 (210 × 297 mm)", "US Letter (8.5 × 11 in)"};
  m_mdPageSizeSelect = new ui::Select(mdPageSizes, m_markdownPanel);
  m_mdPageSizeField =
      new ui::FormField("Page Size", m_mdPageSizeSelect, m_markdownPanel);

  QStringList mdOrientations = {"Portrait", "Landscape"};
  m_mdOrientationSelect = new ui::Select(mdOrientations, m_markdownPanel);
  m_mdOrientationField =
      new ui::FormField("Orientation", m_mdOrientationSelect, m_markdownPanel);

  QStringList mdMargins = {"Compact (18 pt)", "Standard (36 pt)",
                           "Wide (54 pt)", "Extra Wide (72 pt)"};
  m_mdMarginSelect = new ui::Select(mdMargins, m_markdownPanel);
  m_mdMarginSelect->setCurrentIndex(1); // Standard 36 pt
  m_mdMarginField =
      new ui::FormField("Page Margins", m_mdMarginSelect, m_markdownPanel);

  QStringList mdFontSizes = {"9 pt", "10 pt (Default)", "11 pt",
                             "12 pt", "14 pt", "16 pt"};
  m_mdFontSizeSelect = new ui::Select(mdFontSizes, m_markdownPanel);
  m_mdFontSizeSelect->setCurrentIndex(1); // 10 pt
  m_mdFontSizeField =
      new ui::FormField("Base Font Size", m_mdFontSizeSelect, m_markdownPanel);

  mdLayout->addWidget(m_mdPageSizeField, 1, Qt::AlignTop);
  mdLayout->addWidget(m_mdOrientationField, 1, Qt::AlignTop);
  mdLayout->addWidget(m_mdMarginField, 1, Qt::AlignTop);
  mdLayout->addWidget(m_mdFontSizeField, 1, Qt::AlignTop);
  m_optionsStack->addWidget(m_markdownPanel);
}

QString PdfPage::processButtonTextForMode(PdfToolMode mode) const {
  switch (mode) {
  case PdfToolMode::Compress:
    return "Compress PDF";
  case PdfToolMode::Merge:
    return "Merge PDFs";
  case PdfToolMode::Split:
    return "Split PDF";
  case PdfToolMode::Organize:
    return "Save Organized PDF";
  case PdfToolMode::PdfToImages:
    return "Export Pages to Images";
  case PdfToolMode::ImagesToPdf:
    return "Create Merged PDF";
  case PdfToolMode::Grayscale:
    return "Convert PDF to Grayscale";
  case PdfToolMode::MarkdownToPdf:
    return "Export to PDF";
  }
  return "Process PDF";
}

QString PdfPage::cancelButtonTextForMode(PdfToolMode mode) const {
  switch (mode) {
  case PdfToolMode::Compress:
    return "Cancel Compression";
  case PdfToolMode::Merge:
    return "Cancel Merge";
  case PdfToolMode::Split:
    return "Cancel Split";
  case PdfToolMode::Organize:
    return "Cancel Operation";
  case PdfToolMode::PdfToImages:
    return "Cancel Export";
  case PdfToolMode::ImagesToPdf:
    return "Cancel Creation";
  case PdfToolMode::Grayscale:
    return "Cancel Conversion";
  case PdfToolMode::MarkdownToPdf:
    return "Cancel PDF Export";
  }
  return "Cancel";
}

void PdfPage::onToolTabChanged(int index) {
  if (m_isProcessing) {
    return;
  }
  m_currentTool = static_cast<PdfToolMode>(
      qBound(0, index, static_cast<int>(PdfToolMode::MarkdownToPdf)));
  updateToolModeUi();

  if (m_currentTool == PdfToolMode::Merge) {
    syncMergeListFromDropWidget();
  } else if (m_currentTool == PdfToolMode::Organize) {
    const QStringList paths = m_dragWidget->getFilePaths();
    if (!paths.isEmpty() && m_organizedSourcePath != paths.first()) {
      loadOrganizePagesFromSelectedPdf(true);
    }
  } else if (m_currentTool == PdfToolMode::ImagesToPdf) {
    syncImageListFromDropWidget();
  }
}

void PdfPage::updateToolModeUi() {
  const bool isImagesInput = (m_currentTool == PdfToolMode::ImagesToPdf);
  const bool isDocInput = (m_currentTool == PdfToolMode::MarkdownToPdf);
  if (m_dragWidget) {
    m_dragWidget->setVisible(!isImagesInput && !isDocInput);
  }
  if (m_imageDragWidget) {
    m_imageDragWidget->setVisible(isImagesInput);
  }
  if (m_docDragWidget) {
    m_docDragWidget->setVisible(isDocInput);
  }

  if (m_optionsStack) {
    const int stackIdx = static_cast<int>(m_currentTool);
    m_optionsStack->setCurrentIndex(stackIdx);
    const bool hasCustomPanel = (m_currentTool != PdfToolMode::Compress &&
                                 m_currentTool != PdfToolMode::Grayscale);
    m_optionsStack->setVisible(hasCustomPanel);
    if (QWidget *currentWidget = m_optionsStack->currentWidget()) {
      m_optionsStack->setMaximumHeight(
          hasCustomPanel ? currentWidget->sizeHint().height() + 24 : 0);
    }
  }

  if (m_qualityRowWidget) {
    const bool showQualityRow =
        (m_currentTool == PdfToolMode::Compress ||
         m_currentTool == PdfToolMode::Grayscale ||
         (m_currentTool == PdfToolMode::PdfToImages &&
          m_exportImageFormatSelect &&
          m_exportImageFormatSelect->currentIndex() != 0));
    m_qualityRowWidget->setVisible(showQualityRow);
  }
  if (m_presetField) {
    m_presetField->setVisible(m_currentTool == PdfToolMode::Compress ||
                              m_currentTool == PdfToolMode::Grayscale);
  }

  if (m_processButton) {
    m_processButton->setText(processButtonTextForMode(m_currentTool));
  }
  if (m_cancelButton && !m_isProcessing) {
    m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));
  }
}

void PdfPage::onExportFormatChanged(int index) {
  Q_UNUSED(index);
  if (m_currentTool == PdfToolMode::PdfToImages) {
    updateToolModeUi();
  }
}

void PdfPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  const QString hintStyle =
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground));
  for (QLabel *lbl : {m_organizeHintLabel, m_imageOrderHintLabel}) {
    if (lbl) {
      lbl->setFont(t.font(t.sizeXs, QFont::DemiBold));
      lbl->setStyleSheet(hintStyle);
    }
  }

  const QString listStyle = QString("QListWidget {"
                                    "  background-color: %1;"
                                    "  color: %2;"
                                    "  border: 1px solid %3;"
                                    "  border-radius: %4px;"
                                    "  padding: 4px;"
                                    "  outline: none;"
                                    "}"
                                    "QListWidget::item {"
                                    "  padding: 5px 8px;"
                                    "  border-radius: %5px;"
                                    "}"
                                    "QListWidget::item:selected {"
                                    "  background-color: %6;"
                                    "  color: %2;"
                                    "}"
                                    "QListWidget::item:hover {"
                                    "  background-color: %7;"
                                    "}")
                                .arg(ui::StyleHelper::toHexString(c.card))
                                .arg(ui::StyleHelper::toHexString(c.foreground))
                                .arg(ui::StyleHelper::toHexString(c.border))
                                .arg(r.md)
                                .arg(r.sm)
                                .arg(ui::StyleHelper::toHexString(c.accent))
                                .arg(ui::StyleHelper::toHexString(c.secondary));

  for (QListWidget *listWidget : {m_organizePageList, m_imageOrderList}) {
    if (listWidget) {
      listWidget->setFont(t.font(t.sizeSm));
      listWidget->setStyleSheet(listStyle);
    }
  }

  if (m_processButton) {
    m_processButton->setIcon(ui::Icon::get(":/icons/icons/lucide-file-text.svg",
                                           c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(
        ":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void PdfPage::onPresetChanged(int index) {
  if (!m_qualitySlider)
    return;
  m_qualitySlider->blockSignals(true);
  if (index == 1) {
    m_qualitySlider->setValue(20);
  } else if (index == 2) {
    m_qualitySlider->setValue(50);
  } else if (index == 3) {
    m_qualitySlider->setValue(80);
  }
  m_qualitySlider->blockSignals(false);
}

void PdfPage::onQualitySliderChanged() {
  if (!m_presetSelect || !m_qualitySlider)
    return;
  int val = m_qualitySlider->getValue();
  m_presetSelect->blockSignals(true);
  if (val == 20) {
    m_presetSelect->setCurrentIndex(1);
  } else if (val == 50) {
    m_presetSelect->setCurrentIndex(2);
  } else if (val == 80) {
    m_presetSelect->setCurrentIndex(3);
  } else {
    m_presetSelect->setCurrentIndex(0);
  }
  m_presetSelect->blockSignals(false);
}

void PdfPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  if (m_toolTabs) {
    m_toolTabs->setEnabled(!isProcessing);
  }
  if (m_optionsStack) {
    m_optionsStack->setEnabled(!isProcessing);
  }
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  if (m_progressLabel) {
    m_progressLabel->setVisible(isProcessing);
  }
  if (m_progressBar) {
    if (!isProcessing) {
      m_progressBar->setIndeterminate(false);
    }
    m_progressBar->setVisible(isProcessing);
  }
  m_qualitySlider->setEnabled(!isProcessing);
  if (m_presetSelect) {
    m_presetSelect->setEnabled(!isProcessing);
  }
  DropFileWidget *drop = activeDropWidget();
  if (drop) {
    if (isProcessing) {
      drop->beginProcessing();
    } else {
      const bool cancelled =
          m_currentController ? m_currentController->isCancelled() : false;
      drop->finishProcessing(cancelled);
    }
  }
}

void PdfPage::onCancelButtonClicked() {
  if (m_isProcessing && m_currentController) {
    m_currentController->cancel();
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Cancelling...");
  }
}

void PdfPage::onControllerProgress(int current, int total,
                                   const QString &statusText) {
  if (m_progressBar) {
    if (total <= 0) {
      m_progressBar->setIndeterminate(true);
    } else {
      m_progressBar->setIndeterminate(false);
      m_progressBar->setRange(0, total);
      m_progressBar->setValue(current, false);
    }
  }
  if (m_progressLabel) {
    m_progressLabel->setText(statusText);
  }
}

void PdfPage::onControllerFileStatus(const QString &filePath, int status,
                                     const QString &detail) {
  DropFileWidget *drop = activeDropWidget();
  if (drop) {
    drop->setFileStatus(filePath,
                        static_cast<DropFileWidget::FileStatus>(status),
                        detail);
  }
}

void PdfPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  switch (m_currentTool) {
  case PdfToolMode::Compress:
    executeCompress();
    break;
  case PdfToolMode::Merge:
    executeMerge();
    break;
  case PdfToolMode::Split:
    executeSplit();
    break;
  case PdfToolMode::Organize:
    executeOrganize();
    break;
  case PdfToolMode::PdfToImages:
    executePdfToImages();
    break;
  case PdfToolMode::ImagesToPdf:
    executeImagesToPdf();
    break;
  case PdfToolMode::Grayscale:
    executeGrayscale();
    break;
  case PdfToolMode::MarkdownToPdf:
    executeMarkdownPdf();
    break;
  }
}

// ============================================================================
// Tool 0: Compress PDF
// ============================================================================

void PdfPage::executeCompress() {
  const QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select at least one PDF file.",
                    ui::ToastVariant::Warning);
    return;
  }

  const int quality = m_qualitySlider->getValue();
  CompressController::Params params;
  params.quality = quality;

  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    const QString outputPath = QFileDialog::getSaveFileName(
        this, "Save Compressed PDF",
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_compressed.pdf"),
        "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return;
    }

    if (pdfutil::isSamePdf(QFileInfo(sourcePath), QFileInfo(outputPath))) {
      ui::Toast::show(
          this, "Invalid Output File",
          "Choose a different output file. The original PDF must not "
          "be overwritten.",
          ui::ToastVariant::Destructive);
      return;
    }

    params.isBatch = false;
    params.sourcePath = sourcePath;
    params.outputPath = outputPath;
  } else {
    const QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory for Compressed PDFs", QDir::homePath());

    if (outputDir.isEmpty()) {
      return;
    }

    params.isBatch = true;
    params.sourcePaths = sourcePaths;
    params.outputDir = outputDir;
  }

  m_currentController = m_compressController;
  setProcessingState(true);
  m_compressController->start(params);
}

void PdfPage::onCompressFinished(const CompressController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (!result.isBatch) {
    // Single compression
    if (result.status == CompressController::Result::Status::Compressed) {
      const double savedPercent =
          100.0 * (1.0 - static_cast<double>(result.outBytes) / result.origBytes);
      ui::Dialog::info(
          this, "PDF Compressed",
          QString("Saved to: %1\n\nOriginal: %2\nCompressed: %3 "
                  "(%4% smaller)")
              .arg(result.singleOutputPath)
              .arg(DropFileWidget::formatFileSize(result.origBytes))
              .arg(DropFileWidget::formatFileSize(result.outBytes))
              .arg(savedPercent, 0, 'f', 1));
    } else if (result.status ==
               CompressController::Result::Status::Unchanged) {
      ui::Dialog::info(
          this, "No Reduction Achieved",
          "These settings did not produce a smaller PDF. "
          "The original PDF and destination were left unchanged.");
    } else if (result.status ==
               CompressController::Result::Status::Cancelled) {
      ui::Toast::show(this, "Cancelled", "PDF compression was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(this, "PDF Compression Failed",
                      result.errors.isEmpty() ? "Compression failed."
                                              : result.errors.first(),
                      ui::ToastVariant::Destructive);
    }
  } else {
    // Batch compression
    const QString title =
        (result.status == CompressController::Result::Status::Cancelled)
            ? "Batch Compression Cancelled"
            : "Batch Compression Complete";
    QString message =
        QString("%1 PDF(s) compressed.\n%2 PDF(s) unchanged (no output "
                "saved).\n%3 PDF(s) failed.")
            .arg(result.successCount)
            .arg(result.unchangedCount)
            .arg(result.failCount);

    if (result.origBytes > 0 && result.outBytes > 0 &&
        result.successCount > 0) {
      const double diffPct =
          (1.0 - (static_cast<double>(result.outBytes) / result.origBytes)) *
          100.0;
      message +=
          QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
              .arg(DropFileWidget::formatFileSize(result.origBytes))
              .arg(DropFileWidget::formatFileSize(result.outBytes))
              .arg(qAbs(diffPct), 0, 'f', 1)
              .arg(diffPct >= 0 ? "smaller" : "larger");
    }

    if (!result.errors.isEmpty()) {
      message += "\n\n" + result.errors.join("\n");
    }
    const int totalFiles = m_dragWidget->getFilePaths().size();
    if (result.status == CompressController::Result::Status::Cancelled) {
      message += QString("\n%1 PDF(s) not completed due to cancellation.")
                     .arg(totalFiles - result.successCount -
                          result.unchangedCount - result.failCount);
    }
    ui::Dialog::info(this, title, message);
  }
}

// ============================================================================
// Tool 1: Merge PDF
// ============================================================================

void PdfPage::syncMergeListFromDropWidget() {
  if (m_mergePanel && m_dragWidget) {
    m_mergePanel->syncFromPaths(m_dragWidget->getFilePaths());
  }
}

void PdfPage::executeMerge() {
  syncMergeListFromDropWidget();

  const QStringList orderedPaths =
      m_mergePanel ? m_mergePanel->orderedPaths() : QStringList();

  if (orderedPaths.size() < 2) {
    ui::Toast::show(this, "Not Enough PDFs",
                    "Please select at least 2 PDF files to merge.",
                    ui::ToastVariant::Warning);
    return;
  }

  const QFileInfo firstInfo(orderedPaths.first());
  const QString defaultOutput =
      firstInfo.dir().filePath(firstInfo.baseName() + "_merged.pdf");
  const QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Merged PDF", defaultOutput, "PDF Files (*.pdf)");
  if (outputPath.isEmpty()) {
    return;
  }

  const QFileInfo destInfo(outputPath);
  for (const QString &srcPath : orderedPaths) {
    if (pdfutil::isSamePdf(QFileInfo(srcPath), destInfo)) {
      ui::Toast::show(
          this, "Invalid Output File",
          "The merged PDF destination cannot overwrite one of the input PDFs.",
          ui::ToastVariant::Destructive);
      return;
    }
  }

  MergeController::Params params;
  params.inputPaths = orderedPaths;
  params.outputPath = outputPath;

  m_currentController = m_mergeController;
  setProcessingState(true);
  m_mergeController->start(params);
}

void PdfPage::onMergeFinished(const MergeController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "PDF merge was cancelled.",
                    ui::ToastVariant::Default);
  } else if (result.success) {
    ui::Dialog::info(
        this, "PDFs Merged",
        QString("Merged %1 PDFs (%2 total pages) into:\n%3\n\n"
                "Output Size: %4")
            .arg(m_mergePanel ? m_mergePanel->count() : 0)
            .arg(result.totalPages)
            .arg(result.outputPath)
            .arg(DropFileWidget::formatFileSize(result.outputBytes)));
  } else {
    ui::Toast::show(this, "PDF Merge Failed", result.error,
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Tool 2: Split PDF
// ============================================================================

void PdfPage::executeSplit() {
  const QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select at least one PDF file to split.",
                    ui::ToastVariant::Warning);
    return;
  }

  const int splitMode = m_splitPanel ? m_splitPanel->splitMode() : 0;
  const QString rangeSpec =
      m_splitPanel ? m_splitPanel->rangeSpec() : QString();
  const int interval = m_splitPanel ? m_splitPanel->interval() : 1;

  // Pre-validate the first PDF and range syntax before prompting for folder
  if (splitMode == 0) {
    QPdfDocument checkDoc;
    if (checkDoc.load(sourcePaths.first()) != QPdfDocument::Error::None ||
        checkDoc.pageCount() <= 0) {
      ui::Toast::show(this, "Invalid PDF",
                      QString("Cannot open '%1'.")
                          .arg(QFileInfo(sourcePaths.first()).fileName()),
                      ui::ToastVariant::Destructive);
      return;
    }
    QVector<QPair<int, int>> testRanges;
    QString rangeErr;
    if (!pdfutil::parsePageRanges(rangeSpec, checkDoc.pageCount(), testRanges,
                                  rangeErr)) {
      ui::Toast::show(this, "Invalid Page Range", rangeErr,
                      ui::ToastVariant::Warning);
      return;
    }
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Split PDFs", QDir::homePath());
  if (outputDir.isEmpty()) {
    return;
  }

  SplitController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.mode = splitMode;
  params.rangeSpec = rangeSpec;
  params.interval = interval;

  m_currentController = m_splitController;
  setProcessingState(true);
  m_splitController->start(params);
}

void PdfPage::onSplitFinished(const SplitController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "PDF split was cancelled.",
                    ui::ToastVariant::Default);
  } else if (result.createdCount > 0 && result.failedDocs == 0) {
    ui::Dialog::info(this, "PDF Split Complete",
                     QString("Created %1 split PDF file(s) in:\n%2")
                         .arg(result.createdCount)
                         .arg(result.outputDir));
  } else if (result.createdCount > 0) {
    ui::Dialog::info(
        this, "PDF Split Finished with Warnings",
        QString("Created %1 split PDF file(s) in:\n%2\n\nErrors:\n%3")
            .arg(result.createdCount)
            .arg(result.outputDir)
            .arg(result.errors.join("\n")));
  } else {
    ui::Toast::show(this, "PDF Split Failed",
                    result.errors.isEmpty() ? "Could not split PDF."
                                            : result.errors.join("\n"),
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Tool 3: Organize / Rotate / Delete PDF Pages
// ============================================================================

void PdfPage::updateOrganizeItemDisplay(QListWidgetItem *item) const {
  if (!item) {
    return;
  }
  const int origIndex = item->data(Qt::UserRole).toInt();
  const int rot = item->data(Qt::UserRole + 1).toInt();
  const QSizeF ptSize = item->data(Qt::UserRole + 2).toSizeF();
  const QImage baseThumb = item->data(Qt::UserRole + 3).value<QImage>();

  if (!baseThumb.isNull()) {
    const QImage rotatedThumb =
        (rot == 0) ? baseThumb
                   : baseThumb.transformed(QTransform().rotate(rot),
                                           Qt::SmoothTransformation);
    item->setIcon(QIcon(QPixmap::fromImage(rotatedThumb)));
  }

  item->setText(QString("Page %1 (%2 × %3 pt) — Rotation: %4°")
                    .arg(origIndex + 1)
                    .arg(qRound(ptSize.width()))
                    .arg(qRound(ptSize.height()))
                    .arg(rot));
}

void PdfPage::loadOrganizePagesFromSelectedPdf(bool silentIfEmpty) {
  if (!m_organizePageList || !m_dragWidget) {
    return;
  }
  const QStringList paths = m_dragWidget->getFilePaths();
  if (paths.isEmpty()) {
    m_organizePageList->clear();
    m_organizedSourcePath.clear();
    if (!silentIfEmpty) {
      ui::Toast::show(this, "No PDF Selected",
                      "Please add a PDF file in the drop zone first.",
                      ui::ToastVariant::Warning);
    }
    return;
  }

  const QString sourcePath = paths.first();
  QPdfDocument doc;
  const auto err = doc.load(sourcePath);
  if (err != QPdfDocument::Error::None || doc.pageCount() <= 0) {
    m_organizePageList->clear();
    m_organizedSourcePath.clear();
    ui::Toast::show(
        this, "Cannot Load PDF Pages",
        QString("Could not open '%1' (damaged or password-protected).")
            .arg(QFileInfo(sourcePath).fileName()),
        ui::ToastVariant::Destructive);
    return;
  }

  m_organizePageList->clear();
  m_organizedSourcePath = sourcePath;

  for (int i = 0; i < doc.pageCount(); ++i) {
    const QSizeF ptSize = doc.pagePointSize(i);
    const QSize thumbSize(80, 112);
    const QImage thumb = pdfutil::renderPdfPageOnWhite(doc, i, thumbSize);

    QListWidgetItem *item = new QListWidgetItem(m_organizePageList);
    item->setData(Qt::UserRole, i);
    item->setData(Qt::UserRole + 1, 0);
    item->setData(Qt::UserRole + 2, ptSize);
    item->setData(Qt::UserRole + 3, thumb);
    updateOrganizeItemDisplay(item);
  }

  if (m_organizePageList->count() > 0) {
    m_organizePageList->setCurrentRow(0);
  }
}

void PdfPage::rotateSelectedOrganizePages(int deltaDegrees) {
  if (!m_organizePageList) {
    return;
  }
  const QList<QListWidgetItem *> selected = m_organizePageList->selectedItems();
  if (selected.isEmpty()) {
    ui::Toast::show(this, "No Page Selected",
                    "Select one or more pages in the list to rotate.",
                    ui::ToastVariant::Warning);
    return;
  }
  for (QListWidgetItem *item : selected) {
    int rot = (item->data(Qt::UserRole + 1).toInt() + deltaDegrees) % 360;
    if (rot < 0) {
      rot += 360;
    }
    item->setData(Qt::UserRole + 1, rot);
    updateOrganizeItemDisplay(item);
  }
}

void PdfPage::moveSelectedOrganizePage(int direction) {
  if (!m_organizePageList) {
    return;
  }
  const int row = m_organizePageList->currentRow();
  if (row < 0) {
    return;
  }
  const int targetRow = row + direction;
  if (targetRow >= 0 && targetRow < m_organizePageList->count()) {
    QListWidgetItem *item = m_organizePageList->takeItem(row);
    m_organizePageList->insertItem(targetRow, item);
    m_organizePageList->setCurrentRow(targetRow);
  }
}

void PdfPage::deleteSelectedOrganizePages() {
  if (!m_organizePageList) {
    return;
  }
  const QList<QListWidgetItem *> selected = m_organizePageList->selectedItems();
  if (selected.isEmpty()) {
    ui::Toast::show(this, "No Page Selected",
                    "Select one or more pages in the list to delete.",
                    ui::ToastVariant::Warning);
    return;
  }
  if (m_organizePageList->count() - selected.size() < 1) {
    ui::Toast::show(this, "Cannot Delete All Pages",
                    "At least one page must remain in the document.",
                    ui::ToastVariant::Warning);
    return;
  }
  for (QListWidgetItem *item : selected) {
    delete m_organizePageList->takeItem(m_organizePageList->row(item));
  }
}

void PdfPage::resetOrganizePages() { loadOrganizePagesFromSelectedPdf(false); }

void PdfPage::executeOrganize() {
  const QStringList paths = m_dragWidget->getFilePaths();
  if (paths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select a PDF file to organize.",
                    ui::ToastVariant::Warning);
    return;
  }

  if (m_organizedSourcePath != paths.first() ||
      m_organizePageList->count() == 0) {
    loadOrganizePagesFromSelectedPdf(false);
  }
  if (m_organizePageList->count() == 0) {
    return;
  }

  QVector<OrganizeController::PageOp> pageOps;
  pageOps.reserve(m_organizePageList->count());
  for (int i = 0; i < m_organizePageList->count(); ++i) {
    QListWidgetItem *item = m_organizePageList->item(i);
    OrganizeController::PageOp op;
    op.originalPageIndex = item->data(Qt::UserRole).toInt();
    op.rotationDegrees = item->data(Qt::UserRole + 1).toInt();
    pageOps.append(op);
  }

  const QString sourcePath = m_organizedSourcePath;
  const QFileInfo sourceInfo(sourcePath);
  const QString defaultOutput =
      sourceInfo.dir().filePath(sourceInfo.baseName() + "_organized.pdf");
  const QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Organized PDF", defaultOutput, "PDF Files (*.pdf)");
  if (outputPath.isEmpty()) {
    return;
  }

  if (pdfutil::isSamePdf(sourceInfo, QFileInfo(outputPath))) {
    ui::Toast::show(this, "Invalid Output File",
                    "Choose a different output file. The original PDF must not "
                    "be overwritten.",
                    ui::ToastVariant::Destructive);
    return;
  }

  OrganizeController::Params params;
  params.sourcePath = sourcePath;
  params.outputPath = outputPath;
  params.pageOps = pageOps;

  m_currentController = m_organizeController;
  setProcessingState(true);
  m_organizeController->start(params);
}

void PdfPage::onOrganizeFinished(const OrganizeController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "Page organization was cancelled.",
                    ui::ToastVariant::Default);
  } else if (result.success) {
    ui::Dialog::info(
        this, "Organized PDF Saved",
        QString("Saved %1 page(s) to:\n%2\n\nOutput Size: %3")
            .arg(result.pageCount)
            .arg(result.outputPath)
            .arg(DropFileWidget::formatFileSize(result.outputBytes)));
  } else {
    ui::Toast::show(this, "Failed to Save Organized PDF", result.error,
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Tool 4: PDF to Images
// ============================================================================

void PdfPage::executePdfToImages() {
  const QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select at least one PDF file to export.",
                    ui::ToastVariant::Warning);
    return;
  }

  const int fmtIndex =
      m_exportImageFormatSelect ? m_exportImageFormatSelect->currentIndex() : 0;
  const QStringList formats = {"png", "jpg", "webp", "tiff"};
  const QString format = formats.value(fmtIndex, "png");
  const int effectiveQuality =
      (format == "png") ? -1 : m_qualitySlider->getValue();

  const int dpiIndex =
      m_exportDpiSelect ? m_exportDpiSelect->currentIndex() : 2;
  const QVector<int> dpis = {72, 150, 300, 600};
  const int dpi = dpis.value(dpiIndex, 300);

  const QString rangeSpec = m_exportPageRangeInput
                                ? m_exportPageRangeInput->text().trimmed()
                                : QString();

  if (!rangeSpec.isEmpty()) {
    QPdfDocument checkDoc;
    if (checkDoc.load(sourcePaths.first()) != QPdfDocument::Error::None ||
        checkDoc.pageCount() <= 0) {
      ui::Toast::show(this, "Invalid PDF",
                      QString("Cannot open '%1'.")
                          .arg(QFileInfo(sourcePaths.first()).fileName()),
                      ui::ToastVariant::Destructive);
      return;
    }
    QVector<QPair<int, int>> testRanges;
    QString rangeErr;
    if (!pdfutil::parsePageRanges(rangeSpec, checkDoc.pageCount(), testRanges,
                                  rangeErr)) {
      ui::Toast::show(this, "Invalid Page Filter", rangeErr,
                      ui::ToastVariant::Warning);
      return;
    }
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Exported Page Images",
      QDir::homePath());
  if (outputDir.isEmpty()) {
    return;
  }

  PdfToImagesController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.format = format;
  params.dpi = dpi;
  params.rangeFilter = rangeSpec;
  params.quality = effectiveQuality;

  m_currentController = m_pdfToImagesController;
  setProcessingState(true);
  m_pdfToImagesController->start(params);
}

void PdfPage::onPdfToImagesFinished(
    const PdfToImagesController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "Page image export was cancelled.",
                    ui::ToastVariant::Default);
  } else if (result.imagesExported > 0 && result.failedDocs == 0) {
    ui::Dialog::info(
        this, "PDF Pages Exported",
        QString("Exported %1 %2 image(s) at %3 DPI to:\n%4")
            .arg(result.imagesExported)
            .arg(result.format.toUpper())
            .arg(result.dpi)
            .arg(result.outputDir));
  } else if (result.imagesExported > 0) {
    ui::Dialog::info(this, "Export Completed with Warnings",
                     QString("Exported %1 %2 image(s) to:\n%3\n\nErrors:\n%4")
                         .arg(result.imagesExported)
                         .arg(result.format.toUpper())
                         .arg(result.outputDir)
                         .arg(result.errors.join("\n")));
  } else {
    ui::Toast::show(this, "Export Failed",
                    result.errors.isEmpty()
                        ? "Could not export pages to images."
                        : result.errors.join("\n"),
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Tool 5: Images to Merged PDF
// ============================================================================

void PdfPage::syncImageListFromDropWidget() {
  if (!m_imageOrderList || !m_imageDragWidget) {
    return;
  }
  const QStringList dropPaths = m_imageDragWidget->getFilePaths();
  QStringList ordered;
  for (int i = 0; i < m_imageOrderList->count(); ++i) {
    const QString path =
        m_imageOrderList->item(i)->data(Qt::UserRole).toString();
    if (dropPaths.contains(path) && !ordered.contains(path)) {
      ordered.append(path);
    }
  }
  for (const QString &path : dropPaths) {
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext != "pdf" && !ordered.contains(path)) {
      ordered.append(path);
    }
  }

  m_imageOrderList->clear();
  for (const QString &path : ordered) {
    QFileInfo fi(path);
    QListWidgetItem *item =
        new QListWidgetItem(QString("%1 (%2)")
                                .arg(fi.fileName())
                                .arg(DropFileWidget::formatFileSize(fi.size())),
                            m_imageOrderList);
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
  }
}

void PdfPage::moveImageItemUp() {
  if (!m_imageOrderList)
    return;
  const int row = m_imageOrderList->currentRow();
  if (row > 0) {
    QListWidgetItem *item = m_imageOrderList->takeItem(row);
    m_imageOrderList->insertItem(row - 1, item);
    m_imageOrderList->setCurrentRow(row - 1);
  }
}

void PdfPage::moveImageItemDown() {
  if (!m_imageOrderList)
    return;
  const int row = m_imageOrderList->currentRow();
  if (row >= 0 && row < m_imageOrderList->count() - 1) {
    QListWidgetItem *item = m_imageOrderList->takeItem(row);
    m_imageOrderList->insertItem(row + 1, item);
    m_imageOrderList->setCurrentRow(row + 1);
  }
}

void PdfPage::executeImagesToPdf() {
  syncImageListFromDropWidget();

  QStringList orderedImages;
  for (int i = 0; i < m_imageOrderList->count(); ++i) {
    orderedImages.append(
        m_imageOrderList->item(i)->data(Qt::UserRole).toString());
  }

  if (orderedImages.isEmpty()) {
    ui::Toast::show(this, "No Images Selected",
                    "Please select at least one image file (PNG, JPG, WEBP, "
                    "TIFF, BMP, GIF).",
                    ui::ToastVariant::Warning);
    return;
  }

  const QFileInfo firstInfo(orderedImages.first());
  const QString defaultOutput =
      firstInfo.dir().filePath(firstInfo.baseName() + "_merged.pdf");
  const QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Merged PDF from Images", defaultOutput, "PDF Files (*.pdf)");
  if (outputPath.isEmpty()) {
    return;
  }

  const int pageSizeIndex =
      m_imgPdfPageSizeSelect ? m_imgPdfPageSizeSelect->currentIndex() : 0;
  const int orientIndex =
      m_imgPdfOrientationSelect ? m_imgPdfOrientationSelect->currentIndex() : 0;
  const int marginIndex =
      m_imgPdfMarginSelect ? m_imgPdfMarginSelect->currentIndex() : 0;

  ImagesToPdfController::Params params;
  params.imagePaths = orderedImages;
  params.outputPath = outputPath;
  params.pageSizeMode = pageSizeIndex;
  params.orientationMode = orientIndex;
  params.marginMode = marginIndex;

  m_currentController = m_imagesToPdfController;
  setProcessingState(true);
  m_imagesToPdfController->start(params);
}

void PdfPage::onImagesToPdfFinished(
    const ImagesToPdfController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "Image-to-PDF creation was cancelled.",
                    ui::ToastVariant::Default);
  } else if (result.success) {
    ui::Dialog::info(
        this, "Merged PDF Created",
        QString("Combined %1 image(s) into:\n%2\n\nOutput Size: %3")
            .arg(result.pageCount)
            .arg(result.outputPath)
            .arg(DropFileWidget::formatFileSize(result.outputBytes)));
  } else {
    ui::Toast::show(this, "PDF Creation Failed", result.error,
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Tool 6: PDF to Grayscale
// ============================================================================

void PdfPage::executeGrayscale() {
  const QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select at least one PDF file.",
                    ui::ToastVariant::Warning);
    return;
  }

  const int quality = m_qualitySlider->getValue();
  GrayscaleController::Params params;
  params.quality = quality;

  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    const QString outputPath = QFileDialog::getSaveFileName(
        this, "Save Grayscale PDF",
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_grayscale.pdf"),
        "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return;
    }

    if (pdfutil::isSamePdf(QFileInfo(sourcePath), QFileInfo(outputPath))) {
      ui::Toast::show(
          this, "Invalid Output File",
          "Choose a different output file. The original PDF must not "
          "be overwritten.",
          ui::ToastVariant::Destructive);
      return;
    }

    params.isBatch = false;
    params.sourcePath = sourcePath;
    params.outputPath = outputPath;
  } else {
    const QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory for Grayscale PDFs", QDir::homePath());

    if (outputDir.isEmpty()) {
      return;
    }

    params.isBatch = true;
    params.sourcePaths = sourcePaths;
    params.outputDir = outputDir;
  }

  m_currentController = m_grayscaleController;
  setProcessingState(true);
  m_grayscaleController->start(params);
}

void PdfPage::onGrayscaleFinished(const GrayscaleController::Result &result) {
  setProcessingState(false);
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (!result.isBatch) {
    // Single grayscale
    if (result.success) {
      ui::Dialog::info(
          this, "Grayscale PDF Saved",
          QString("Saved to: %1\n\nOriginal Size: %2\nGrayscale Size: %3")
              .arg(result.singleOutputPath)
              .arg(DropFileWidget::formatFileSize(result.origBytes))
              .arg(DropFileWidget::formatFileSize(result.outBytes)));
    } else if (result.cancelled) {
      ui::Toast::show(this, "Cancelled",
                      "Grayscale conversion was cancelled.",
                      ui::ToastVariant::Default);
    } else {
      ui::Toast::show(this, "Grayscale Conversion Failed", result.error,
                      ui::ToastVariant::Destructive);
    }
  } else {
    // Batch grayscale
    const QString title = result.cancelled
                              ? "Batch Grayscale Conversion Cancelled"
                              : "Batch Grayscale Conversion Complete";
    QString message = QString("%1 PDF(s) converted to grayscale.\n"
                              "%2 PDF(s) failed.")
                          .arg(result.successCount)
                          .arg(result.failCount);

    if (result.origBytes > 0 && result.outBytes > 0 &&
        result.successCount > 0) {
      message +=
          QString("\n\nTotal Original: %1\nTotal Grayscale: %2")
              .arg(DropFileWidget::formatFileSize(result.origBytes))
              .arg(DropFileWidget::formatFileSize(result.outBytes));
    }
    if (!result.errors.isEmpty()) {
      message += "\n\n" + result.errors.join("\n");
    }
    const int totalFiles = m_dragWidget->getFilePaths().size();
    if (result.cancelled) {
      message += QString("\n%1 PDF(s) not completed due to cancellation.")
                     .arg(totalFiles - result.successCount - result.failCount);
    }
    ui::Dialog::info(this, title, message);
  }
}

// ============================================================================
// Tool 7: Markdown / HTML to PDF
// ============================================================================

void PdfPage::executeMarkdownPdf() {
  const QStringList sourcePaths = m_docDragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(
        this, "No Document Selected",
        "Please select at least one Markdown (.md) or HTML (.html) file.",
        ui::ToastVariant::Warning);
    return;
  }

  const QString defaultDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Generated PDFs", defaultDir);
  if (outputDir.isEmpty()) {
    return;
  }

  const QVector<int> marginOptions = {18, 36, 54, 72};
  const QVector<int> fontOptions = {9, 10, 11, 12, 14, 16};

  MarkdownPdfController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.useLetterPageSize =
      (m_mdPageSizeSelect && m_mdPageSizeSelect->currentIndex() == 1);
  params.landscape =
      (m_mdOrientationSelect && m_mdOrientationSelect->currentIndex() == 1);
  params.marginPt = marginOptions.value(
      m_mdMarginSelect ? m_mdMarginSelect->currentIndex() : 1, 36);
  params.baseFontPt = fontOptions.value(
      m_mdFontSizeSelect ? m_mdFontSizeSelect->currentIndex() : 1, 10);

  m_currentController = m_markdownController;
  setProcessingState(true);
  m_markdownController->start(params);
}

void PdfPage::onMarkdownPdfFinished(
    const MarkdownPdfController::Result &result) {
  setProcessingState(false);
  m_currentController = nullptr;
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "Document PDF export was cancelled.",
                    ui::ToastVariant::Default);
    return;
  }

  if (result.pdfsCreated > 0) {
    QString msg = QString("Exported %1 document(s) to PDF in:\n%2")
                      .arg(result.pdfsCreated)
                      .arg(result.outputDir);
    if (result.failedFiles > 0) {
      msg += QString("\n\n%1 file(s) failed:\n%2")
                 .arg(result.failedFiles)
                 .arg(result.error);
    }
    ui::Dialog::info(this, "PDF Export Complete", msg);
  } else {
    ui::Toast::show(this, "PDF Export Failed",
                    result.error.isEmpty()
                        ? "Failed to export Markdown/HTML to PDF."
                        : result.error,
                    ui::ToastVariant::Destructive);
  }
}
