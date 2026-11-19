#include "pages/main_page.h"
#include "ui-kit/theme/i18n.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QPushButton>
#include <QtGlobal>

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(12);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  initControllers();
  updateToolModeUi();
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainPage::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &MainPage::retranslateUi);
  applyThemeStyles();
  retranslateUi();
}

MainPage::~MainPage() {
  for (PdfToolController *ctrl :
       {m_convertImagePanel ? m_convertImagePanel->controller() : nullptr,
        m_svgPanel ? m_svgPanel->controller() : nullptr,
        m_icoPanel ? m_icoPanel->controller() : nullptr}) {
    if (ctrl && ctrl->isRunning()) {
      ctrl->cancelAndWait();
    }
  }
}

void MainPage::initControllers() {
  for (PdfToolController *ctrl :
       {m_convertImagePanel ? m_convertImagePanel->controller() : nullptr,
        m_svgPanel ? m_svgPanel->controller() : nullptr,
        m_icoPanel ? m_icoPanel->controller() : nullptr}) {
    if (!ctrl) {
      continue;
    }
    connect(ctrl, &PdfToolController::progressChanged, this,
            &MainPage::onControllerProgress, Qt::QueuedConnection);
    connect(ctrl, &PdfToolController::fileStatusChanged, this,
            &MainPage::onControllerFileStatus, Qt::QueuedConnection);
  }

  if (m_convertImagePanel) {
    connect(m_convertImagePanel, &ConvertImagePanel::executionFinished, this,
            [this]() { setProcessingState(false); });
  }
  if (m_svgPanel) {
    connect(m_svgPanel, &SvgOptionsPanel::executionFinished, this,
            [this]() { setProcessingState(false); });
  }
  if (m_icoPanel) {
    connect(m_icoPanel, &IcoPackagerPanel::executionFinished, this,
            [this]() { setProcessingState(false); });
  }
}

bool MainPage::isAnyJobRunning() const {
  return m_isProcessing ||
         (m_convertImagePanel && m_convertImagePanel->controller() &&
          m_convertImagePanel->controller()->isRunning()) ||
         (m_svgPanel && m_svgPanel->controller() &&
          m_svgPanel->controller()->isRunning()) ||
         (m_icoPanel && m_icoPanel->controller() &&
          m_icoPanel->controller()->isRunning());
}

DropFileWidget *MainPage::activeDropWidget() const {
  switch (m_currentTool) {
  case ImageToolMode::ConvertImage:
  case ImageToolMode::IcoPackager:
    return m_dragWidget;
  case ImageToolMode::SvgToRaster:
    return m_svgDragWidget;
  case ImageToolMode::Base64DataUri:
    return nullptr;
  }
  return m_dragWidget;
}

void MainPage::setupImageLayout() {
  m_toolTabs = new ui::Tabs(ui::TabVariant::Segmented, this);
  m_toolTabs->addTab(tr("Convert Image"));
  m_toolTabs->addTab(tr("SVG Vector"));
  m_toolTabs->addTab(tr("App Icon (.ico)"));
  m_toolTabs->addTab(tr("Base64 Data URI"));
  connect(m_toolTabs, &ui::Tabs::currentChanged, this,
          &MainPage::onToolTabChanged);

  m_optionsStack = new QStackedWidget(this);
  setupToolPanels();

  m_topWorkspaceStack = new QStackedWidget(this);
  m_dragWidget = new DropFileWidget(m_topWorkspaceStack, "Image");
  m_svgDragWidget = new DropFileWidget(m_topWorkspaceStack, "SVG");
  m_base64Card = new Base64WorkspaceCard(m_topWorkspaceStack);
  m_base64Card->setOptionsPanel(m_base64Panel);

  m_topWorkspaceStack->addWidget(m_dragWidget);    // index 0
  m_topWorkspaceStack->addWidget(m_svgDragWidget); // index 1
  m_topWorkspaceStack->addWidget(m_base64Card);    // index 2

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignCenter);
  m_progressLabel->setVisible(false);

  m_progressBar = new ui::Progress(0, this);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button(tr("Convert Files"), ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton =
      new ui::Button(tr("Cancel Conversion"), ui::ButtonVariant::Destructive,
                     ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &MainPage::onCancelButtonClicked);

  mainLayout->addWidget(m_toolTabs);
  mainLayout->addWidget(m_topWorkspaceStack);
  mainLayout->addWidget(m_optionsStack);
  mainLayout->addWidget(m_progressLabel);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
}

void MainPage::setupToolPanels() {
  // Page 0: Convert Image
  m_convertImagePanel = new ConvertImagePanel(m_optionsStack);
  m_optionsStack->addWidget(m_convertImagePanel);

  // Page 1: SVG Vector to Raster / PDF
  m_svgPanel = new SvgOptionsPanel(m_optionsStack);
  m_optionsStack->addWidget(m_svgPanel);

  // Page 2: App Icon (.ico) Packager
  m_icoPanel = new IcoPackagerPanel(m_optionsStack);
  m_optionsStack->addWidget(m_icoPanel);

  // Page 3: Base64 Data URI options
  m_base64Panel = new Base64OptionsPanel(m_optionsStack);
  m_optionsStack->addWidget(m_base64Panel);
}

QString MainPage::processButtonTextForMode(ImageToolMode mode) const {
  switch (mode) {
  case ImageToolMode::ConvertImage:
    return tr("Convert Files");
  case ImageToolMode::SvgToRaster:
    return tr("Convert SVG Files");
  case ImageToolMode::IcoPackager:
    return tr("Export .ico Icons");
  case ImageToolMode::Base64DataUri:
    return tr("Copy Base64 to Clipboard");
  }
  return tr("Convert Files");
}

QString MainPage::cancelButtonTextForMode(ImageToolMode mode) const {
  switch (mode) {
  case ImageToolMode::ConvertImage:
    return tr("Cancel Conversion");
  case ImageToolMode::SvgToRaster:
    return tr("Cancel SVG Conversion");
  case ImageToolMode::IcoPackager:
    return tr("Cancel Icon Export");
  case ImageToolMode::Base64DataUri:
    return tr("Cancel");
  }
  return tr("Cancel Conversion");
}

void MainPage::onToolTabChanged(int index) {
  if (isAnyJobRunning()) {
    return;
  }
  m_currentTool = static_cast<ImageToolMode>(
      qBound(0, index, static_cast<int>(ImageToolMode::Base64DataUri)));
  updateToolModeUi();
}

void MainPage::updateToolModeUi() {
  if (m_topWorkspaceStack) {
    int topIdx = 0;
    if (m_currentTool == ImageToolMode::SvgToRaster) {
      topIdx = 1;
    } else if (m_currentTool == ImageToolMode::Base64DataUri) {
      topIdx = 2;
    }
    m_topWorkspaceStack->setCurrentIndex(topIdx);
  }

  if (m_optionsStack) {
    m_optionsStack->setCurrentIndex(static_cast<int>(m_currentTool));
  }

  if (m_processButton) {
    m_processButton->setText(processButtonTextForMode(m_currentTool));
  }
  if (m_cancelButton && !m_isProcessing) {
    m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));
  }
}

void MainPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_processButton) {
    m_processButton->setIcon(
        ui::Icon::get(":/icons/icons/lucide-refresh-cw.svg",
                      c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(
        ":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void MainPage::retranslateUi() {
  if (m_toolTabs) {
    m_toolTabs->setTabText(0, tr("Convert Image"));
    m_toolTabs->setTabText(1, tr("SVG Vector"));
    m_toolTabs->setTabText(2, tr("App Icon (.ico)"));
    m_toolTabs->setTabText(3, tr("Base64 Data URI"));
  }
  if (m_processButton) {
    m_processButton->setText(processButtonTextForMode(m_currentTool));
  }
  if (m_cancelButton) {
    if (m_isProcessing && !m_cancelButton->isEnabled()) {
      m_cancelButton->setText(tr("Cancelling..."));
    } else {
      m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));
    }
  }
}

void MainPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  if (m_toolTabs) {
    m_toolTabs->setEnabled(!isProcessing);
  }
  if (m_optionsStack) {
    m_optionsStack->setEnabled(!isProcessing);
  }
  if (m_processButton) {
    m_processButton->setVisible(!isProcessing);
  }
  if (m_cancelButton) {
    m_cancelButton->setVisible(isProcessing);
    m_cancelButton->setEnabled(isProcessing);
    if (!isProcessing) {
      m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));
    }
  }
  if (m_progressLabel) {
    m_progressLabel->setVisible(isProcessing);
    if (!isProcessing) {
      m_progressLabel->clear();
    }
  }
  if (m_progressBar) {
    if (!isProcessing) {
      m_progressBar->setIndeterminate(false);
      m_progressBar->setValue(0, false);
    }
    m_progressBar->setVisible(isProcessing);
  }

  DropFileWidget *drop = activeDropWidget();
  if (drop) {
    if (isProcessing) {
      drop->beginProcessing();
    } else {
      const bool cancelled =
          m_currentController && m_currentController->isCancelled();
      drop->finishProcessing(cancelled);
    }
  }

  if (m_convertImagePanel) {
    m_convertImagePanel->syncProcessingState(isProcessing);
  }
  if (m_svgPanel) {
    m_svgPanel->syncProcessingState(isProcessing);
  }

  if (!isProcessing) {
    m_currentController = nullptr;
  }
}

void MainPage::onCancelButtonClicked() {
  if (!m_isProcessing) {
    return;
  }
  if (m_currentController) {
    m_currentController->cancel();
  }
  if (m_cancelButton) {
    m_cancelButton->setText(tr("Cancelling..."));
    m_cancelButton->setEnabled(false);
  }
}

void MainPage::onControllerProgress(int current, int total,
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
  if (m_progressLabel && !statusText.isEmpty()) {
    m_progressLabel->setText(statusText);
  }
}

void MainPage::onControllerFileStatus(const QString &filePath, int status,
                                      const QString &detail) {
  DropFileWidget *drop = activeDropWidget();
  if (drop) {
    drop->setFileStatus(
        filePath, static_cast<DropFileWidget::FileStatus>(status), detail);
  }
}

void MainPage::onProcessButtonClicked() {
  if (isAnyJobRunning()) {
    return;
  }

  switch (m_currentTool) {
  case ImageToolMode::ConvertImage:
    if (m_convertImagePanel && m_dragWidget &&
        m_convertImagePanel->execute(m_dragWidget->getFilePaths())) {
      m_currentController = m_convertImagePanel->controller();
      setProcessingState(true);
    }
    break;
  case ImageToolMode::SvgToRaster:
    if (m_svgPanel && m_svgDragWidget &&
        m_svgPanel->execute(m_svgDragWidget->getFilePaths())) {
      m_currentController = m_svgPanel->controller();
      setProcessingState(true);
    }
    break;
  case ImageToolMode::IcoPackager:
    if (m_icoPanel && m_dragWidget &&
        m_icoPanel->execute(m_dragWidget->getFilePaths())) {
      m_currentController = m_icoPanel->controller();
      setProcessingState(true);
    }
    break;
  case ImageToolMode::Base64DataUri:
    if (m_base64Card) {
      m_base64Card->copyToClipboard();
    }
    break;
  }
}
