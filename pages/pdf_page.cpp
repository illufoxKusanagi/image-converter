#include "pages/pdf_page.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPushButton>
#include <QtGlobal>

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

  m_qualityPanel = new PdfQualityPanel(this);

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
  mainLayout->addWidget(m_qualityPanel);
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
  // Page 0: Compress (uses m_qualityPanel)
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
  m_organizePanel = new OrganizePdfPanel(m_optionsStack);
  connect(m_organizePanel, &OrganizePdfPanel::reloadRequested, this,
          [this]() { loadOrganizePagesFromSelectedPdf(false); });
  m_optionsStack->addWidget(m_organizePanel);

  // Page 4: PDF to Images
  m_pdfToImagesPanel = new PdfToImagesPanel(m_optionsStack);
  connect(m_pdfToImagesPanel, &PdfToImagesPanel::exportFormatChanged, this,
          &PdfPage::onExportFormatChanged);
  m_optionsStack->addWidget(m_pdfToImagesPanel);

  // Page 5: Images to Merged PDF
  m_imagesToPdfPanel = new ImagesToPdfPanel(m_optionsStack);
  connect(m_imagesToPdfPanel, &ImagesToPdfPanel::syncRequested, this,
          &PdfPage::syncImageListFromDropWidget);
  m_optionsStack->addWidget(m_imagesToPdfPanel);

  // Page 6: Grayscale (uses m_qualityPanel)
  QWidget *grayscaleDummy = new QWidget(m_optionsStack);
  m_optionsStack->addWidget(grayscaleDummy);

  // Page 7: Markdown / HTML to PDF
  m_markdownPanel = new MarkdownPdfPanel(m_optionsStack);
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
    if (m_dragWidget && m_organizePanel) {
      const QStringList paths = m_dragWidget->getFilePaths();
      if (!paths.isEmpty() && m_organizePanel->sourcePath() != paths.first()) {
        loadOrganizePagesFromSelectedPdf(true);
      }
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

  if (m_qualityPanel) {
    const bool showQualityRow =
        (m_currentTool == PdfToolMode::Compress ||
         m_currentTool == PdfToolMode::Grayscale ||
         (m_currentTool == PdfToolMode::PdfToImages &&
          m_pdfToImagesPanel &&
          m_pdfToImagesPanel->formatIndex() != 0));
    m_qualityPanel->setVisible(showQualityRow);
    m_qualityPanel->setPresetVisible(m_currentTool == PdfToolMode::Compress ||
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
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
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
  if (m_qualityPanel) {
    m_qualityPanel->setControlsEnabled(!isProcessing);
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

  const int quality = m_qualityPanel ? m_qualityPanel->quality() : 50;
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

void PdfPage::loadOrganizePagesFromSelectedPdf(bool silentIfEmpty) {
  if (!m_organizePanel || !m_dragWidget) {
    return;
  }
  m_organizePanel->loadFromPaths(m_dragWidget->getFilePaths(), silentIfEmpty);
}

void PdfPage::executeOrganize() {
  const QStringList paths = m_dragWidget->getFilePaths();
  if (paths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select a PDF file to organize.",
                    ui::ToastVariant::Warning);
    return;
  }

  if (!m_organizePanel) {
    return;
  }

  if (m_organizePanel->sourcePath() != paths.first() ||
      m_organizePanel->pageCount() == 0) {
    loadOrganizePagesFromSelectedPdf(false);
  }
  if (m_organizePanel->pageCount() == 0) {
    return;
  }

  const QVector<OrganizeController::PageOp> pageOps = m_organizePanel->pageOps();
  const QString sourcePath = m_organizePanel->sourcePath();
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

  const QString format = m_pdfToImagesPanel
                             ? m_pdfToImagesPanel->selectedFormat()
                             : QString("png");
  const int effectiveQuality =
      (format == "png") ? -1
                        : (m_qualityPanel ? m_qualityPanel->quality() : 50);

  const int dpi = m_pdfToImagesPanel ? m_pdfToImagesPanel->selectedDpi() : 300;
  const QString rangeSpec =
      m_pdfToImagesPanel ? m_pdfToImagesPanel->pageRangeSpec() : QString();

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
  if (m_imagesToPdfPanel && m_imageDragWidget) {
    m_imagesToPdfPanel->syncFromPaths(m_imageDragWidget->getFilePaths());
  }
}

void PdfPage::executeImagesToPdf() {
  syncImageListFromDropWidget();

  const QStringList orderedImages =
      m_imagesToPdfPanel ? m_imagesToPdfPanel->orderedImagePaths()
                         : QStringList();

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
      m_imagesToPdfPanel ? m_imagesToPdfPanel->pageSizeMode() : 0;
  const int orientIndex =
      m_imagesToPdfPanel ? m_imagesToPdfPanel->orientationMode() : 0;
  const int marginIndex =
      m_imagesToPdfPanel ? m_imagesToPdfPanel->marginMode() : 0;

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

  const int quality = m_qualityPanel ? m_qualityPanel->quality() : 50;
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

  MarkdownPdfController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.useLetterPageSize =
      m_markdownPanel ? m_markdownPanel->useLetterPageSize() : false;
  params.landscape = m_markdownPanel ? m_markdownPanel->landscape() : false;
  params.marginPt = m_markdownPanel ? m_markdownPanel->marginPt() : 36;
  params.baseFontPt = m_markdownPanel ? m_markdownPanel->baseFontPt() : 10;

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
