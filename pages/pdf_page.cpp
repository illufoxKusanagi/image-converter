#include "pages/pdf_page.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
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
           m_qualityPanel ? m_qualityPanel->compressController() : nullptr,
           m_mergePanel ? m_mergePanel->controller() : nullptr,
           m_splitPanel ? m_splitPanel->controller() : nullptr,
           m_organizePanel ? m_organizePanel->controller() : nullptr,
           m_pdfToImagesPanel ? m_pdfToImagesPanel->controller() : nullptr,
           m_imagesToPdfPanel ? m_imagesToPdfPanel->controller() : nullptr,
           m_qualityPanel ? m_qualityPanel->grayscaleController() : nullptr,
           m_markdownPanel ? m_markdownPanel->controller() : nullptr}) {
    if (ctrl && ctrl->isRunning()) {
      ctrl->cancelAndWait();
    }
  }
}

void PdfPage::initControllers() {
  for (PdfToolController *ctrl : {
           m_qualityPanel ? m_qualityPanel->compressController() : nullptr,
           m_mergePanel ? m_mergePanel->controller() : nullptr,
           m_splitPanel ? m_splitPanel->controller() : nullptr,
           m_organizePanel ? m_organizePanel->controller() : nullptr,
           m_pdfToImagesPanel ? m_pdfToImagesPanel->controller() : nullptr,
           m_imagesToPdfPanel ? m_imagesToPdfPanel->controller() : nullptr,
           m_qualityPanel ? m_qualityPanel->grayscaleController() : nullptr,
           m_markdownPanel ? m_markdownPanel->controller() : nullptr}) {
    if (!ctrl) {
      continue;
    }
    connect(ctrl, &PdfToolController::progressChanged, this,
            &PdfPage::onControllerProgress, Qt::QueuedConnection);
    connect(ctrl, &PdfToolController::fileStatusChanged, this,
            &PdfPage::onControllerFileStatus, Qt::QueuedConnection);
  }

  auto connectPanelFinished = [this](auto *panel, auto signal) {
    if (panel) {
      connect(panel, signal, this, [this]() { setProcessingState(false); });
    }
  };
  connectPanelFinished(m_qualityPanel, &PdfQualityPanel::executionFinished);
  connectPanelFinished(m_mergePanel, &MergePdfPanel::executionFinished);
  connectPanelFinished(m_splitPanel, &SplitPdfPanel::executionFinished);
  connectPanelFinished(m_organizePanel, &OrganizePdfPanel::executionFinished);
  connectPanelFinished(m_pdfToImagesPanel,
                       &PdfToImagesPanel::executionFinished);
  connectPanelFinished(m_imagesToPdfPanel,
                       &ImagesToPdfPanel::executionFinished);
  connectPanelFinished(m_markdownPanel, &MarkdownPdfPanel::executionFinished);
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
  connect(m_mergePanel, &MergePdfPanel::syncRequested, this, [this]() {
    if (m_mergePanel && m_dragWidget) {
      m_mergePanel->syncFromPaths(m_dragWidget->getFilePaths());
    }
  });
  m_optionsStack->addWidget(m_mergePanel);

  // Page 2: Split PDF
  m_splitPanel = new SplitPdfPanel(m_optionsStack);
  m_optionsStack->addWidget(m_splitPanel);

  // Page 3: Organize / Rotate / Delete PDF Pages
  m_organizePanel = new OrganizePdfPanel(m_optionsStack);
  connect(m_organizePanel, &OrganizePdfPanel::reloadRequested, this, [this]() {
    if (m_organizePanel && m_dragWidget) {
      m_organizePanel->loadFromPaths(m_dragWidget->getFilePaths(), false);
    }
  });
  m_optionsStack->addWidget(m_organizePanel);

  // Page 4: PDF to Images
  m_pdfToImagesPanel = new PdfToImagesPanel(m_optionsStack);
  connect(m_pdfToImagesPanel, &PdfToImagesPanel::exportFormatChanged, this,
          &PdfPage::updateToolModeUi);
  m_optionsStack->addWidget(m_pdfToImagesPanel);

  // Page 5: Images to Merged PDF
  m_imagesToPdfPanel = new ImagesToPdfPanel(m_optionsStack);
  connect(m_imagesToPdfPanel, &ImagesToPdfPanel::syncRequested, this,
          [this]() {
            if (m_imagesToPdfPanel && m_imageDragWidget) {
              m_imagesToPdfPanel->syncFromPaths(
                  m_imageDragWidget->getFilePaths());
            }
          });
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
    if (m_mergePanel && m_dragWidget) {
      m_mergePanel->syncFromPaths(m_dragWidget->getFilePaths());
    }
  } else if (m_currentTool == PdfToolMode::Organize) {
    if (m_dragWidget && m_organizePanel) {
      const QStringList paths = m_dragWidget->getFilePaths();
      if (!paths.isEmpty() && m_organizePanel->sourcePath() != paths.first()) {
        m_organizePanel->loadFromPaths(paths, true);
      }
    }
  } else if (m_currentTool == PdfToolMode::ImagesToPdf) {
    if (m_imagesToPdfPanel && m_imageDragWidget) {
      m_imagesToPdfPanel->syncFromPaths(m_imageDragWidget->getFilePaths());
    }
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
  if (!isProcessing) {
    if (m_cancelButton) {
      m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));
    }
    m_currentController = nullptr;
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

  bool started = false;
  PdfToolController *startedController = nullptr;

  switch (m_currentTool) {
  case PdfToolMode::Compress:
    if (m_qualityPanel && m_dragWidget &&
        m_qualityPanel->executeCompress(m_dragWidget->getFilePaths())) {
      startedController = m_qualityPanel->compressController();
      started = true;
    }
    break;
  case PdfToolMode::Merge:
    if (m_mergePanel && m_dragWidget &&
        m_mergePanel->execute(m_dragWidget->getFilePaths())) {
      startedController = m_mergePanel->controller();
      started = true;
    }
    break;
  case PdfToolMode::Split:
    if (m_splitPanel && m_dragWidget &&
        m_splitPanel->execute(m_dragWidget->getFilePaths())) {
      startedController = m_splitPanel->controller();
      started = true;
    }
    break;
  case PdfToolMode::Organize:
    if (m_organizePanel && m_dragWidget &&
        m_organizePanel->execute(m_dragWidget->getFilePaths())) {
      startedController = m_organizePanel->controller();
      started = true;
    }
    break;
  case PdfToolMode::PdfToImages:
    if (m_pdfToImagesPanel && m_dragWidget &&
        m_pdfToImagesPanel->execute(
            m_dragWidget->getFilePaths(),
            m_qualityPanel ? m_qualityPanel->quality() : 50)) {
      startedController = m_pdfToImagesPanel->controller();
      started = true;
    }
    break;
  case PdfToolMode::ImagesToPdf:
    if (m_imagesToPdfPanel && m_imageDragWidget &&
        m_imagesToPdfPanel->execute(m_imageDragWidget->getFilePaths())) {
      startedController = m_imagesToPdfPanel->controller();
      started = true;
    }
    break;
  case PdfToolMode::Grayscale:
    if (m_qualityPanel && m_dragWidget &&
        m_qualityPanel->executeGrayscale(m_dragWidget->getFilePaths())) {
      startedController = m_qualityPanel->grayscaleController();
      started = true;
    }
    break;
  case PdfToolMode::MarkdownToPdf:
    if (m_markdownPanel && m_docDragWidget &&
        m_markdownPanel->execute(m_docDragWidget->getFilePaths())) {
      startedController = m_markdownPanel->controller();
      started = true;
    }
    break;
  }

  if (started) {
    m_currentController = startedController;
    setProcessingState(true);
  }
}
