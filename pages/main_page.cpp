#include "pages/main_page.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QBuffer>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImageReader>
#include <QIntValidator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QSaveFile>
#include <QSet>
#include <QSvgRenderer>
#include <QtConcurrent/QtConcurrent>

namespace {

QImage loadRasterOrSvgImage(const QString &path,
                            bool forceOpaqueWhite = false) {
  const QFileInfo fi(path);
  if (fi.suffix().compare("svg", Qt::CaseInsensitive) == 0) {
    QSvgRenderer renderer(fi.absoluteFilePath());
    if (!renderer.isValid()) {
      return QImage();
    }
    QSize baseSize = renderer.defaultSize();
    if (!baseSize.isValid() || baseSize.width() <= 0 ||
        baseSize.height() <= 0) {
      baseSize = renderer.viewBoxF().size().toSize();
    }
    if (!baseSize.isValid() || baseSize.width() <= 0 ||
        baseSize.height() <= 0) {
      baseSize = QSize(512, 512);
    }
    if (baseSize.width() > 16384 || baseSize.height() > 16384) {
      baseSize.scale(16384, 16384, Qt::KeepAspectRatio);
    }
    baseSize.setWidth(qBound(1, baseSize.width(), 16384));
    baseSize.setHeight(qBound(1, baseSize.height(), 16384));

    QImage img(baseSize, forceOpaqueWhite ? QImage::Format_RGB32
                                          : QImage::Format_ARGB32_Premultiplied);
    if (img.isNull()) {
      return QImage();
    }
    img.fill(forceOpaqueWhite ? Qt::white : Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter,
                    QRectF(0.0, 0.0, baseSize.width(), baseSize.height()));
    painter.end();
    return img;
  }

  QImageReader reader(fi.absoluteFilePath());
  reader.setAutoTransform(true);
  QImage img = reader.read();
  if (img.isNull()) {
    img = QImage(fi.absoluteFilePath());
  }
  if (!img.isNull() && forceOpaqueWhite && img.hasAlphaChannel()) {
    QImage opaque(img.size(), QImage::Format_RGB32);
    if (!opaque.isNull()) {
      opaque.fill(Qt::white);
      QPainter painter(&opaque);
      painter.drawImage(0, 0, img);
      painter.end();
      return opaque;
    }
  }
  return img;
}

} // namespace

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)),
      m_targetImageExtension(DropFileWidget::ImageExtension::JPG) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(12);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  initControllers();
  if (m_targetExtensionSelect) {
    onImageTargetExtensionChanged();
  }
  updateToolModeUi();
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainPage::applyThemeStyles);
  applyThemeStyles();
}

MainPage::~MainPage() {
  m_isCancelled = true;
  if (m_futureWatcher && m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
  for (PdfToolController *ctrl :
       {static_cast<PdfToolController *>(m_svgController),
        static_cast<PdfToolController *>(m_icoController)}) {
    if (ctrl && ctrl->isRunning()) {
      ctrl->cancelAndWait();
    }
  }
}

void MainPage::initControllers() {
  m_svgController = new SvgConvertController(this);
  m_icoController = new IcoExportController(this);

  for (PdfToolController *ctrl :
       {static_cast<PdfToolController *>(m_svgController),
        static_cast<PdfToolController *>(m_icoController)}) {
    connect(ctrl, &PdfToolController::progressChanged, this,
            &MainPage::onControllerProgress, Qt::QueuedConnection);
    connect(ctrl, &PdfToolController::fileStatusChanged, this,
            &MainPage::onControllerFileStatus, Qt::QueuedConnection);
  }

  connect(m_svgController, &SvgConvertController::finished, this,
          &MainPage::onSvgConvertFinished, Qt::QueuedConnection);
  connect(m_icoController, &IcoExportController::finished, this,
          &MainPage::onIcoExportFinished, Qt::QueuedConnection);
}

bool MainPage::isAnyJobRunning() const {
  return m_isProcessing ||
         (m_futureWatcher && m_futureWatcher->isRunning()) ||
         (m_svgController && m_svgController->isRunning()) ||
         (m_icoController && m_icoController->isRunning());
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

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"JPG",  "JPEG", "PNG", "WEBP",
                                  "TIFF", "BMP",  "GIF", "PDF"};
  m_targetExtensionSelect = new ui::Select(extensionOptions, m_convertImagePanel);
  m_targetExtensionField = new ui::FormField(
      "Target Format", m_targetExtensionSelect, m_convertImagePanel);
  connect(m_targetExtensionSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onImageTargetExtensionChanged);
}

void MainPage::setupImageLayout() {
  m_toolTabs = new ui::Tabs(ui::TabVariant::Segmented, this);
  m_toolTabs->addTab("Convert Image");
  m_toolTabs->addTab("SVG Vector");
  m_toolTabs->addTab("App Icon (.ico)");
  m_toolTabs->addTab("Base64 Data URI");
  connect(m_toolTabs, &ui::Tabs::currentChanged, this,
          &MainPage::onToolTabChanged);

  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  m_topWorkspaceStack = new QStackedWidget(this);
  m_dragWidget = new DropFileWidget(m_topWorkspaceStack, "Image",
                                    m_qualitySlider, &m_targetImageExtension);
  m_svgDragWidget = new DropFileWidget(m_topWorkspaceStack, "SVG");

  m_base64Card = new ui::Card(
      "Image ↔ Base64 Data URI Workspace",
      "Load an image or SVG to generate inline Data URIs, HTML, CSS, or JSON "
      "snippets — or paste Base64 below to decode back to an image file.",
      m_topWorkspaceStack);

  QWidget *b64Content = new QWidget(m_base64Card);
  QVBoxLayout *b64Layout = new QVBoxLayout(b64Content);
  b64Layout->setContentsMargins(0, 0, 0, 0);
  b64Layout->setSpacing(8);

  QHBoxLayout *b64Toolbar = new QHBoxLayout();
  b64Toolbar->setContentsMargins(0, 0, 0, 0);
  b64Toolbar->setSpacing(8);

  m_base64LoadBtn =
      new ui::Button("Load Image / SVG...", ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Small, b64Content);
  m_base64CopyBtn =
      new ui::Button("Copy to Clipboard", ui::ButtonVariant::Outline,
                     ui::ButtonSize::Small, b64Content);
  m_base64DecodeSaveBtn = new ui::Button(
      "Decode Base64 & Save Image...", ui::ButtonVariant::Outline,
      ui::ButtonSize::Small, b64Content);
  m_base64StatsLabel =
      new QLabel("No image loaded — select a file or paste Base64 below",
                 b64Content);

  connect(m_base64LoadBtn, &QPushButton::clicked, this,
          &MainPage::onBase64LoadFileClicked);
  connect(m_base64CopyBtn, &QPushButton::clicked, this,
          &MainPage::onBase64CopyClicked);
  connect(m_base64DecodeSaveBtn, &QPushButton::clicked, this,
          &MainPage::onBase64DecodeSaveClicked);

  b64Toolbar->addWidget(m_base64LoadBtn);
  b64Toolbar->addWidget(m_base64CopyBtn);
  b64Toolbar->addWidget(m_base64DecodeSaveBtn);
  b64Toolbar->addWidget(m_base64StatsLabel, 1, Qt::AlignRight | Qt::AlignVCenter);

  m_base64TextEdit = new QPlainTextEdit(b64Content);
  m_base64TextEdit->setPlaceholderText(
      "Click 'Load Image / SVG...' to encode a file into Base64 / Data URI, or "
      "paste a data:image/...;base64,... URI or raw Base64 payload here and "
      "click 'Decode Base64 & Save Image...'.");
  m_base64TextEdit->setFixedHeight(130);

  b64Layout->addLayout(b64Toolbar);
  b64Layout->addWidget(m_base64TextEdit);
  m_base64Card->setContentWidget(b64Content);

  m_topWorkspaceStack->addWidget(m_dragWidget);    // index 0
  m_topWorkspaceStack->addWidget(m_svgDragWidget); // index 1
  m_topWorkspaceStack->addWidget(m_base64Card);    // index 2

  m_optionsStack = new QStackedWidget(this);
  setupToolPanels();

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignCenter);
  m_progressLabel->setVisible(false);

  m_progressBar = new ui::Progress(0, this);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Convert Files", ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton =
      new ui::Button("Cancel Conversion", ui::ButtonVariant::Destructive,
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
  m_convertImagePanel = new QWidget(m_optionsStack);
  QHBoxLayout *attributeLayout = new QHBoxLayout(m_convertImagePanel);
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_targetExtensionField, 1, Qt::AlignTop);
  m_optionsStack->addWidget(m_convertImagePanel);

  // Page 1: SVG Vector to Raster / PDF
  m_svgPanel = new QWidget(m_optionsStack);
  QVBoxLayout *svgRootLayout = new QVBoxLayout(m_svgPanel);
  svgRootLayout->setContentsMargins(0, 0, 0, 0);
  svgRootLayout->setSpacing(10);

  QHBoxLayout *svgTopRow = new QHBoxLayout();
  svgTopRow->setContentsMargins(0, 0, 0, 0);
  svgTopRow->setSpacing(12);

  QStringList svgFormats = {"PNG", "WEBP", "JPG", "PDF"};
  m_svgFormatSelect = new ui::Select(svgFormats, m_svgPanel);
  m_svgFormatField =
      new ui::FormField("Output Format", m_svgFormatSelect, m_svgPanel);
  connect(m_svgFormatSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onSvgFormatChanged);

  QStringList svgScales = {"1x (Original Size)", "2x (Retina / HiDPI)",
                           "4x (Ultra HD)", "Custom Dimensions (W × H)"};
  m_svgScaleSelect = new ui::Select(svgScales, m_svgPanel);
  m_svgScaleSelect->setCurrentIndex(1); // Default 2x crisp export
  m_svgScaleField =
      new ui::FormField("Vector Scale Preset", m_svgScaleSelect, m_svgPanel);
  connect(m_svgScaleSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onSvgScaleModeChanged);

  m_svgCustomWidthInput = new ui::Input("512", m_svgPanel);
  m_svgCustomWidthInput->setText("512");
  m_svgCustomWidthInput->setSuffixText("px");
  if (m_svgCustomWidthInput->lineEdit()) {
    m_svgCustomWidthInput->lineEdit()->setValidator(
        new QIntValidator(1, 16384, m_svgCustomWidthInput));
  }
  m_svgCustomWidthField =
      new ui::FormField("Custom Width", m_svgCustomWidthInput, m_svgPanel);

  m_svgCustomHeightInput = new ui::Input("512", m_svgPanel);
  m_svgCustomHeightInput->setText("512");
  m_svgCustomHeightInput->setSuffixText("px");
  if (m_svgCustomHeightInput->lineEdit()) {
    m_svgCustomHeightInput->lineEdit()->setValidator(
        new QIntValidator(1, 16384, m_svgCustomHeightInput));
  }
  m_svgCustomHeightField =
      new ui::FormField("Custom Height", m_svgCustomHeightInput, m_svgPanel);

  svgTopRow->addWidget(m_svgFormatField, 1, Qt::AlignTop);
  svgTopRow->addWidget(m_svgScaleField, 2, Qt::AlignTop);
  svgTopRow->addWidget(m_svgCustomWidthField, 1, Qt::AlignTop);
  svgTopRow->addWidget(m_svgCustomHeightField, 1, Qt::AlignTop);

  QHBoxLayout *svgBottomRow = new QHBoxLayout();
  svgBottomRow->setContentsMargins(0, 0, 0, 0);
  svgBottomRow->setSpacing(16);

  m_svgQualitySlider = new SliderWidget(m_svgPanel, "Raster Quality");
  m_svgQualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_svgQualitySlider->setValue(90);

  m_svgKeepAspectSwitch = new ui::Switch(true, m_svgPanel);
  m_svgKeepAspectLabel = new QLabel("Preserve Aspect Ratio", m_svgPanel);

  m_svgWhiteBgSwitch = new ui::Switch(false, m_svgPanel);
  m_svgWhiteBgLabel = new QLabel("White Background", m_svgPanel);

  QHBoxLayout *aspectToggleRow = new QHBoxLayout();
  aspectToggleRow->setContentsMargins(0, 0, 0, 0);
  aspectToggleRow->setSpacing(8);
  aspectToggleRow->addWidget(m_svgKeepAspectSwitch);
  aspectToggleRow->addWidget(m_svgKeepAspectLabel);

  QHBoxLayout *bgToggleRow = new QHBoxLayout();
  bgToggleRow->setContentsMargins(0, 0, 0, 0);
  bgToggleRow->setSpacing(8);
  bgToggleRow->addWidget(m_svgWhiteBgSwitch);
  bgToggleRow->addWidget(m_svgWhiteBgLabel);

  svgBottomRow->addWidget(m_svgQualitySlider, 1, Qt::AlignVCenter);
  svgBottomRow->addLayout(aspectToggleRow);
  svgBottomRow->addLayout(bgToggleRow);

  svgRootLayout->addLayout(svgTopRow);
  svgRootLayout->addLayout(svgBottomRow);
  m_optionsStack->addWidget(m_svgPanel);

  onSvgFormatChanged(0);
  onSvgScaleModeChanged(m_svgScaleSelect->currentIndex());

  // Page 2: App Icon (.ico) Packager
  m_icoPanel = new QWidget(m_optionsStack);
  QVBoxLayout *icoLayout = new QVBoxLayout(m_icoPanel);
  icoLayout->setContentsMargins(0, 0, 0, 0);
  icoLayout->setSpacing(8);

  m_icoHintLabel = new QLabel(
      "Checked icon resolutions are rendered with smooth aspect-preserving "
      "scaling and packed into a single multi-resolution .ico file:",
      m_icoPanel);
  m_icoHintLabel->setWordWrap(true);

  QHBoxLayout *icoSizesRow = new QHBoxLayout();
  icoSizesRow->setContentsMargins(0, 0, 0, 0);
  icoSizesRow->setSpacing(16);

  const QVector<int> icoSizes = {16, 32, 48, 64, 128, 256};
  for (int sz : icoSizes) {
    ui::Switch *sw = new ui::Switch(true, m_icoPanel);
    QLabel *lbl = new QLabel(QString("%1×%1 px").arg(sz), m_icoPanel);
    QHBoxLayout *pairLayout = new QHBoxLayout();
    pairLayout->setContentsMargins(0, 0, 0, 0);
    pairLayout->setSpacing(6);
    pairLayout->addWidget(sw);
    pairLayout->addWidget(lbl);
    icoSizesRow->addLayout(pairLayout);
    m_icoSizeToggles.push_back({sz, sw, lbl});
  }
  icoSizesRow->addStretch(1);

  icoLayout->addWidget(m_icoHintLabel);
  icoLayout->addLayout(icoSizesRow);
  m_optionsStack->addWidget(m_icoPanel);

  // Page 3: Base64 Data URI options
  m_base64Panel = new QWidget(m_optionsStack);
  QHBoxLayout *b64OptLayout = new QHBoxLayout(m_base64Panel);
  b64OptLayout->setContentsMargins(0, 0, 0, 0);
  b64OptLayout->setSpacing(12);

  QStringList snippetFormats = {"Raw Data URI (data:image/...;base64,...)",
                                "Raw Base64 Payload (no prefix)",
                                "HTML <img> Tag",
                                "CSS background-image",
                                "JSON Object Payload"};
  m_base64SnippetSelect = new ui::Select(snippetFormats, m_base64Panel);
  m_base64SnippetField = new ui::FormField(
      "Output Snippet Format", m_base64SnippetSelect, m_base64Panel);

  QStringList encodeFormats = {"Keep Original File Bytes",
                               "Re-encode as PNG",
                               "Re-encode as WEBP",
                               "Re-encode as JPG"};
  m_base64EncodeSelect = new ui::Select(encodeFormats, m_base64Panel);
  m_base64EncodeField = new ui::FormField(
      "Encode Format", m_base64EncodeSelect, m_base64Panel);

  connect(m_base64SnippetSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onBase64OptionChanged);
  connect(m_base64EncodeSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onBase64OptionChanged);

  b64OptLayout->addWidget(m_base64SnippetField, 1, Qt::AlignTop);
  b64OptLayout->addWidget(m_base64EncodeField, 1, Qt::AlignTop);
  m_optionsStack->addWidget(m_base64Panel);
}

QString MainPage::processButtonTextForMode(ImageToolMode mode) const {
  switch (mode) {
  case ImageToolMode::ConvertImage:
    return "Convert Files";
  case ImageToolMode::SvgToRaster:
    return "Convert SVG Files";
  case ImageToolMode::IcoPackager:
    return "Export .ico Icons";
  case ImageToolMode::Base64DataUri:
    return "Copy Base64 to Clipboard";
  }
  return "Convert Files";
}

QString MainPage::cancelButtonTextForMode(ImageToolMode mode) const {
  switch (mode) {
  case ImageToolMode::ConvertImage:
    return "Cancel Conversion";
  case ImageToolMode::SvgToRaster:
    return "Cancel SVG Conversion";
  case ImageToolMode::IcoPackager:
    return "Cancel Icon Export";
  case ImageToolMode::Base64DataUri:
    return "Cancel";
  }
  return "Cancel Conversion";
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
    const int optIdx = static_cast<int>(m_currentTool);
    m_optionsStack->setCurrentIndex(optIdx);
  }

  if (m_processButton) {
    m_processButton->setText(processButtonTextForMode(m_currentTool));
  }
  if (m_cancelButton && !m_isProcessing) {
    m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));
  }
}

void MainPage::onSvgFormatChanged(int index) {
  const bool isLossy = (index == 1 || index == 2); // WEBP or JPG
  const bool isJpg = (index == 2);
  if (m_svgQualitySlider) {
    m_svgQualitySlider->setEnabled(isLossy && !m_isProcessing);
    if (index == 0) {
      m_svgQualitySlider->setTitle("PNG (Lossless - Quality N/A)");
    } else if (index == 3) {
      m_svgQualitySlider->setTitle("Vector PDF (Resolution Independent)");
    } else {
      m_svgQualitySlider->setTitle("Raster Quality");
    }
  }
  if (m_svgWhiteBgSwitch) {
    if (isJpg) {
      m_svgWhiteBgSwitch->setChecked(true);
      m_svgWhiteBgSwitch->setEnabled(false);
    } else {
      m_svgWhiteBgSwitch->setEnabled(!m_isProcessing);
    }
  }
}

void MainPage::onSvgScaleModeChanged(int index) {
  const bool isCustom = (index == 3);
  if (m_svgCustomWidthInput) {
    m_svgCustomWidthInput->setEnabled(isCustom && !m_isProcessing);
  }
  if (m_svgCustomHeightInput) {
    m_svgCustomHeightInput->setEnabled(isCustom && !m_isProcessing);
  }
  if (m_svgKeepAspectSwitch) {
    m_svgKeepAspectSwitch->setEnabled(isCustom && !m_isProcessing);
  }
}

void MainPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  const QString labelStyle =
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground));
  for (QLabel *lbl : {m_svgKeepAspectLabel, m_svgWhiteBgLabel}) {
    if (lbl) {
      lbl->setFont(t.font(t.sizeSm, QFont::Medium));
      lbl->setStyleSheet(labelStyle);
    }
  }

  if (m_icoHintLabel) {
    m_icoHintLabel->setFont(t.font(t.sizeXs, QFont::Medium));
    m_icoHintLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  for (const IcoSizeToggle &item : m_icoSizeToggles) {
    if (item.label) {
      item.label->setFont(t.font(t.sizeSm, QFont::DemiBold));
      item.label->setStyleSheet(labelStyle);
    }
  }

  if (m_base64StatsLabel) {
    m_base64StatsLabel->setFont(t.font(t.sizeXs, QFont::Medium));
    m_base64StatsLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_base64TextEdit) {
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPixelSize(t.sizeXs);
    m_base64TextEdit->setFont(monoFont);
    m_base64TextEdit->setStyleSheet(
        QString("QPlainTextEdit {"
                "  background-color: %1;"
                "  color: %2;"
                "  border: 1px solid %3;"
                "  border-radius: %4px;"
                "  padding: 6px 8px;"
                "}")
            .arg(ui::StyleHelper::toHexString(c.background))
            .arg(ui::StyleHelper::toHexString(c.foreground))
            .arg(ui::StyleHelper::toHexString(c.border))
            .arg(r.md));
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

void MainPage::setProcessingState(bool isProcessing) {
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
      m_progressBar->setValue(0, false);
    }
    m_progressBar->setVisible(isProcessing);
  }

  DropFileWidget *drop = activeDropWidget();
  if (drop) {
    if (isProcessing) {
      drop->beginProcessing();
    } else {
      const bool cancelled = m_currentController
                                 ? m_currentController->isCancelled()
                                 : m_isCancelled.load();
      drop->finishProcessing(cancelled);
    }
  }

  if (!isProcessing) {
    if (m_currentTool == ImageToolMode::ConvertImage) {
      onImageTargetExtensionChanged();
    } else if (m_currentTool == ImageToolMode::SvgToRaster &&
               m_svgFormatSelect && m_svgScaleSelect) {
      onSvgFormatChanged(m_svgFormatSelect->currentIndex());
      onSvgScaleModeChanged(m_svgScaleSelect->currentIndex());
    }
  } else {
    m_qualitySlider->setEnabled(false);
  }
}

void MainPage::onCancelButtonClicked() {
  if (!m_isProcessing) {
    return;
  }
  m_isCancelled = true;
  if (m_currentController) {
    m_currentController->cancel();
  }
  m_cancelButton->setText("Cancelling...");
  m_cancelButton->setEnabled(false);
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
    drop->setFileStatus(filePath,
                        static_cast<DropFileWidget::FileStatus>(status),
                        detail);
  }
}

void MainPage::onProcessButtonClicked() {
  if (isAnyJobRunning()) {
    return;
  }

  switch (m_currentTool) {
  case ImageToolMode::ConvertImage:
    executeConvertImage();
    break;
  case ImageToolMode::SvgToRaster:
    executeSvgConvert();
    break;
  case ImageToolMode::IcoPackager:
    executeIcoExport();
    break;
  case ImageToolMode::Base64DataUri:
    onBase64CopyClicked();
    break;
  }
}

// ============================================================================
// Mode 1: SVG Vector to Raster / PDF
// ============================================================================

void MainPage::executeSvgConvert() {
  const QStringList sourcePaths = m_svgDragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No SVG Files Selected",
                    "Please select at least one .svg file.",
                    ui::ToastVariant::Warning);
    return;
  }

  const QString defaultDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Converted SVG Files", defaultDir);
  if (outputDir.isEmpty()) {
    return;
  }

  const QStringList fmtKeys = {"png", "webp", "jpg", "pdf"};
  const int fmtIdx =
      m_svgFormatSelect ? m_svgFormatSelect->currentIndex() : 0;

  SvgConvertController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.format = fmtKeys.value(fmtIdx, "png");
  params.scaleMode =
      m_svgScaleSelect ? m_svgScaleSelect->currentIndex() : 0;
  params.customWidth =
      m_svgCustomWidthInput ? m_svgCustomWidthInput->text().toInt() : 512;
  params.customHeight =
      m_svgCustomHeightInput ? m_svgCustomHeightInput->text().toInt() : 512;
  if (params.customWidth <= 0) {
    params.customWidth = 512;
  }
  if (params.customHeight <= 0) {
    params.customHeight = 512;
  }
  params.preserveAspectRatio =
      (params.scaleMode != 3) ||
      (m_svgKeepAspectSwitch && m_svgKeepAspectSwitch->isChecked());
  params.whiteBackground =
      (params.format == "jpg") ||
      (m_svgWhiteBgSwitch && m_svgWhiteBgSwitch->isChecked());
  params.quality =
      m_svgQualitySlider ? m_svgQualitySlider->getValue() : 90;

  m_currentController = m_svgController;
  m_isCancelled = false;
  setProcessingState(true);
  m_svgController->start(params);
}

void MainPage::onSvgConvertFinished(
    const SvgConvertController::Result &result) {
  setProcessingState(false);
  m_currentController = nullptr;
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "SVG conversion was cancelled.",
                    ui::ToastVariant::Default);
    return;
  }

  if (result.filesConverted > 0) {
    QString msg = QString("Converted %1 SVG file(s) to %2 in:\n%3")
                      .arg(result.filesConverted)
                      .arg(result.format.toUpper())
                      .arg(result.outputDir);
    if (result.failedFiles > 0) {
      msg += QString("\n\n%1 file(s) failed:\n%2")
                 .arg(result.failedFiles)
                 .arg(result.error);
    }
    ui::Dialog::info(this, "SVG Conversion Complete", msg);
  } else {
    ui::Toast::show(this, "SVG Conversion Failed",
                    result.error.isEmpty()
                        ? "Failed to convert SVG files."
                        : result.error,
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Mode 2: Multi-Size App Icon (.ico) Packager
// ============================================================================

void MainPage::executeIcoExport() {
  const QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No Image Selected",
                    "Please select at least one image or SVG file.",
                    ui::ToastVariant::Warning);
    return;
  }

  QVector<int> selectedSizes;
  for (const IcoSizeToggle &item : m_icoSizeToggles) {
    if (item.toggle && item.toggle->isChecked()) {
      selectedSizes.append(item.size);
    }
  }
  if (selectedSizes.isEmpty()) {
    ui::Toast::show(this, "No Icon Sizes Selected",
                    "Please enable at least one icon resolution (e.g. 32×32 or 256×256).",
                    ui::ToastVariant::Warning);
    return;
  }

  const QString defaultDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for .ico Files", defaultDir);
  if (outputDir.isEmpty()) {
    return;
  }

  IcoExportController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.sizes = selectedSizes;

  m_currentController = m_icoController;
  m_isCancelled = false;
  setProcessingState(true);
  m_icoController->start(params);
}

void MainPage::onIcoExportFinished(const IcoExportController::Result &result) {
  setProcessingState(false);
  m_currentController = nullptr;
  m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

  if (result.cancelled) {
    ui::Toast::show(this, "Cancelled", "Icon export was cancelled.",
                    ui::ToastVariant::Default);
    return;
  }

  if (result.iconsCreated > 0) {
    QStringList sizeLabels;
    for (int sz : result.sizes) {
      sizeLabels.append(QString("%1×%1").arg(sz));
    }
    QString msg =
        QString("Created %1 multi-resolution .ico file(s) (%2 px) in:\n%3")
            .arg(result.iconsCreated)
            .arg(sizeLabels.join(", "))
            .arg(result.outputDir);
    if (result.failedFiles > 0) {
      msg += QString("\n\n%1 file(s) failed:\n%2")
                 .arg(result.failedFiles)
                 .arg(result.error);
    }
    ui::Dialog::info(this, "Icon Export Complete", msg);
  } else {
    ui::Toast::show(this, "Icon Export Failed",
                    result.error.isEmpty() ? "Failed to generate .ico files."
                                           : result.error,
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Mode 3: Image ↔ Base64 Data URI
// ============================================================================

void MainPage::onBase64LoadFileClicked() {
  const QString filePath = QFileDialog::getOpenFileName(
      this, "Select Image or SVG to Encode", QDir::homePath(),
      "Images & Vector Files (*.png *.jpg *.jpeg *.webp *.svg *.gif *.bmp *.ico);;All Files (*)");
  if (filePath.isEmpty()) {
    return;
  }
  loadFileIntoBase64(filePath);
}

void MainPage::onBase64OptionChanged() {
  if (!m_base64LoadedFilePath.isEmpty() &&
      QFile::exists(m_base64LoadedFilePath)) {
    loadFileIntoBase64(m_base64LoadedFilePath);
  }
}

void MainPage::loadFileIntoBase64(const QString &filePath) {
  const QFileInfo fi(filePath);
  QFile rawFile(fi.absoluteFilePath());
  if (!rawFile.open(QIODevice::ReadOnly)) {
    ui::Toast::show(this, "Read Error",
                    QString("Cannot open '%1'.").arg(fi.fileName()),
                    ui::ToastVariant::Destructive);
    return;
  }
  const QByteArray rawBytes = rawFile.readAll();
  rawFile.close();

  const int encodeIdx =
      m_base64EncodeSelect ? m_base64EncodeSelect->currentIndex() : 0;
  const QString ext = fi.suffix().toLower();

  QSize imgDim;
  if (ext == "svg") {
    QSvgRenderer svg(rawBytes);
    if (svg.isValid()) {
      imgDim = svg.defaultSize();
      if (!imgDim.isValid() || imgDim.width() <= 0 || imgDim.height() <= 0) {
        imgDim = svg.viewBoxF().size().toSize();
      }
    }
  } else {
    QImageReader reader(fi.absoluteFilePath());
    reader.setAutoTransform(true);
    imgDim = reader.size();
    if (!imgDim.isValid()) {
      const QImage loaded = reader.read();
      imgDim = loaded.size();
    }
  }

  QByteArray payloadBytes;
  QString mimeType = "image/png";

  if (encodeIdx == 0) {
    payloadBytes = rawBytes;
    if (ext == "svg") {
      mimeType = "image/svg+xml";
    } else if (ext == "png") {
      mimeType = "image/png";
    } else if (ext == "jpg" || ext == "jpeg") {
      mimeType = "image/jpeg";
    } else if (ext == "webp") {
      mimeType = "image/webp";
    } else if (ext == "gif") {
      mimeType = "image/gif";
    } else if (ext == "bmp") {
      mimeType = "image/bmp";
    } else if (ext == "ico") {
      mimeType = "image/x-icon";
    } else {
      mimeType = "application/octet-stream";
    }
  } else {
    const bool forceWhite = (encodeIdx == 3); // JPG
    const QImage img = loadRasterOrSvgImage(fi.absoluteFilePath(), forceWhite);
    if (img.isNull()) {
      ui::Toast::show(this, "Encode Error",
                      "Could not decode image for re-encoding.",
                      ui::ToastVariant::Destructive);
      return;
    }
    imgDim = img.size();
    QBuffer buf(&payloadBytes);
    bool encodedOk = buf.open(QIODevice::WriteOnly);
    if (encodedOk) {
      if (encodeIdx == 1) {
        mimeType = "image/png";
        encodedOk = img.save(&buf, "PNG", -1);
      } else if (encodeIdx == 2) {
        mimeType = "image/webp";
        encodedOk = img.save(&buf, "WEBP", 90);
      } else {
        mimeType = "image/jpeg";
        encodedOk = img.save(&buf, "JPG", 90);
      }
      buf.close();
    }
    if (!encodedOk || payloadBytes.isEmpty()) {
      ui::Toast::show(this, "Encode Error",
                      "Failed to re-encode image into target format.",
                      ui::ToastVariant::Destructive);
      return;
    }
  }

  m_base64LoadedFilePath = fi.absoluteFilePath();

  const QString b64 = QString::fromLatin1(payloadBytes.toBase64());
  const QString dataUri = QString("data:%1;base64,%2").arg(mimeType, b64);

  const int snippetIdx =
      m_base64SnippetSelect ? m_base64SnippetSelect->currentIndex() : 0;
  QString formattedOutput = dataUri;

  if (snippetIdx == 1) {
    formattedOutput = b64;
  } else if (snippetIdx == 2) {
    const int w = imgDim.width() > 0 ? imgDim.width() : 256;
    const int h = imgDim.height() > 0 ? imgDim.height() : 256;
    formattedOutput =
        QString("<img src=\"%1\" alt=\"%2\" width=\"%3\" height=\"%4\" />")
            .arg(dataUri, fi.completeBaseName().toHtmlEscaped())
            .arg(w)
            .arg(h);
  } else if (snippetIdx == 3) {
    formattedOutput = QString("background-image: url(\"%1\");\n"
                              "background-repeat: no-repeat;\n"
                              "background-size: contain;")
                          .arg(dataUri);
  } else if (snippetIdx == 4) {
    QJsonObject obj;
    obj["fileName"] = fi.fileName();
    obj["mimeType"] = mimeType;
    if (imgDim.isValid() && imgDim.width() > 0 && imgDim.height() > 0) {
      obj["width"] = imgDim.width();
      obj["height"] = imgDim.height();
    }
    obj["byteSize"] = static_cast<qint64>(payloadBytes.size());
    obj["dataUri"] = dataUri;
    obj["base64"] = b64;
    formattedOutput =
        QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented))
            .trimmed();
  }

  if (m_base64TextEdit) {
    m_base64TextEdit->setPlainText(formattedOutput);
  }
  if (m_base64StatsLabel) {
    const QString dimStr =
        (imgDim.isValid() && imgDim.width() > 0 && imgDim.height() > 0)
            ? QString("%1×%2 px • ").arg(imgDim.width()).arg(imgDim.height())
            : QString();
    m_base64StatsLabel->setText(
        QString("%1 (%2%3 → %4 chars)")
            .arg(fi.fileName(), dimStr,
                 DropFileWidget::formatFileSize(payloadBytes.size()),
                 QString::number(formattedOutput.size())));
  }
}

void MainPage::onBase64CopyClicked() {
  const QString text =
      m_base64TextEdit ? m_base64TextEdit->toPlainText().trimmed() : QString();
  if (text.isEmpty()) {
    ui::Toast::show(this, "Nothing to Copy",
                    "Load an image or enter a Base64 snippet first.",
                    ui::ToastVariant::Warning);
    return;
  }
  if (QClipboard *cb = QGuiApplication::clipboard()) {
    cb->setText(text);
    ui::Toast::show(this, "Copied to Clipboard",
                    QString("Copied %1 characters to clipboard.")
                        .arg(text.size()),
                    ui::ToastVariant::Success);
  }
}

void MainPage::onBase64DecodeSaveClicked() {
  QString rawText =
      m_base64TextEdit ? m_base64TextEdit->toPlainText().trimmed() : QString();
  if (rawText.isEmpty()) {
    ui::Toast::show(this, "Empty Base64 Input",
                    "Paste a Data URI or Base64 string into the editor first.",
                    ui::ToastVariant::Warning);
    return;
  }

  if (rawText.startsWith('{')) {
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(rawText.toUtf8());
    if (jsonDoc.isObject()) {
      const QJsonObject obj = jsonDoc.object();
      if (obj.contains("dataUri") && obj["dataUri"].isString()) {
        rawText = obj["dataUri"].toString().trimmed();
      } else if (obj.contains("base64") && obj["base64"].isString()) {
        rawText = obj["base64"].toString().trimmed();
      }
    }
  }

  QString mimeHint;
  QString base64Payload = rawText;

  const int dataIdx = rawText.indexOf("data:", 0, Qt::CaseInsensitive);
  const int b64MarkerIdx = rawText.indexOf(";base64,", 0, Qt::CaseInsensitive);
  if (dataIdx >= 0 && b64MarkerIdx > dataIdx) {
    mimeHint =
        rawText.mid(dataIdx + 5, b64MarkerIdx - (dataIdx + 5)).trimmed().toLower();
    int startPos = b64MarkerIdx + 8;
    int endPos = rawText.size();
    for (int i = startPos; i < rawText.size(); ++i) {
      const QChar ch = rawText.at(i);
      if (ch == '"' || ch == '\'' || ch == ')' || ch == ';') {
        endPos = i;
        break;
      }
    }
    base64Payload = rawText.mid(startPos, endPos - startPos).trimmed();
  }

  const QByteArray decodedBytes =
      QByteArray::fromBase64(base64Payload.toLatin1());
  if (decodedBytes.isEmpty()) {
    ui::Toast::show(this, "Invalid Base64",
                    "The input could not be decoded from Base64.",
                    ui::ToastVariant::Destructive);
    return;
  }

  bool isSvgPayload = (mimeHint == "image/svg+xml");
  if (!isSvgPayload) {
    const QByteArray trimmedSample = decodedBytes.left(512).trimmed();
    if (trimmedSample.startsWith("<svg") ||
        trimmedSample.startsWith("<?xml")) {
      QSvgRenderer testSvg(decodedBytes);
      isSvgPayload = testSvg.isValid();
    }
  }

  QImage decodedImg;
  if (!isSvgPayload) {
    decodedImg = QImage::fromData(decodedBytes);
    if (decodedImg.isNull()) {
      QSvgRenderer fallbackSvg(decodedBytes);
      if (fallbackSvg.isValid()) {
        isSvgPayload = true;
      } else {
        ui::Toast::show(
            this, "Unsupported Image Data",
            "Decoded bytes do not form a valid raster image or SVG document.",
            ui::ToastVariant::Destructive);
        return;
      }
    }
  }

  QString defaultExt = "png";
  if (isSvgPayload) {
    defaultExt = "svg";
  } else if (mimeHint == "image/jpeg" || mimeHint == "image/jpg") {
    defaultExt = "jpg";
  } else if (mimeHint == "image/webp") {
    defaultExt = "webp";
  } else if (mimeHint == "image/gif") {
    defaultExt = "gif";
  } else if (mimeHint == "image/bmp") {
    defaultExt = "bmp";
  } else if (mimeHint == "image/x-icon" ||
             mimeHint == "image/vnd.microsoft.icon") {
    defaultExt = "ico";
  }

  const QString defaultPath =
      QDir(QDir::homePath()).filePath("decoded_image." + defaultExt);
  QString saveFilter;
  if (isSvgPayload) {
    saveFilter =
        "SVG Vector (*.svg);;PNG Image (*.png);;WEBP Image (*.webp);;JPG Image (*.jpg)";
  } else if (defaultExt == "ico") {
    saveFilter =
        "ICO Icon (*.ico);;PNG Image (*.png);;WEBP Image (*.webp);;JPG Image (*.jpg);;BMP Image (*.bmp)";
  } else {
    saveFilter =
        "PNG Image (*.png);;JPG Image (*.jpg);;WEBP Image (*.webp);;BMP Image (*.bmp);;TIFF Image (*.tiff)";
  }

  QString outPath = QFileDialog::getSaveFileName(
      this, "Save Decoded Image", defaultPath, saveFilter);
  if (outPath.isEmpty()) {
    return;
  }
  if (QFileInfo(outPath).suffix().isEmpty()) {
    outPath += "." + defaultExt;
  }

  const QString outExt = QFileInfo(outPath).suffix().toLower();
  QSaveFile saveFile(outPath);
  saveFile.setDirectWriteFallback(false);
  bool savedOk = false;

  if (saveFile.open(QIODevice::WriteOnly)) {
    if (isSvgPayload && outExt == "svg") {
      savedOk = (saveFile.write(decodedBytes) == decodedBytes.size());
    } else if (isSvgPayload) {
      QSvgRenderer renderer(decodedBytes);
      QSize targetSz = renderer.defaultSize();
      if (!targetSz.isValid() || targetSz.width() <= 0 ||
          targetSz.height() <= 0) {
        targetSz = QSize(512, 512);
      }
      QImage rendered(targetSz, outExt == "jpg"
                                    ? QImage::Format_RGB32
                                    : QImage::Format_ARGB32_Premultiplied);
      rendered.fill(outExt == "jpg" ? Qt::white : Qt::transparent);
      QPainter painter(&rendered);
      painter.setRenderHint(QPainter::Antialiasing, true);
      renderer.render(&painter,
                      QRectF(0, 0, targetSz.width(), targetSz.height()));
      painter.end();
      savedOk = rendered.save(&saveFile, outExt.toUpper().toUtf8().constData(),
                              outExt == "png" ? -1 : 90);
    } else if (outExt == defaultExt) {
      savedOk = (saveFile.write(decodedBytes) == decodedBytes.size());
    } else {
      savedOk = decodedImg.save(
          &saveFile, outExt.toUpper().toUtf8().constData(),
          outExt == "png" ? -1 : 90);
    }
    if (savedOk) {
      savedOk = saveFile.commit();
    }
  }

  if (savedOk) {
    ui::Toast::show(this, "Saved Decoded Image",
                    QString("Saved to %1").arg(QFileInfo(outPath).fileName()),
                    ui::ToastVariant::Success);
  } else {
    ui::Toast::show(this, "Save Failed",
                    QString("Could not save '%1'.").arg(outPath),
                    ui::ToastVariant::Destructive);
  }
}

// ============================================================================
// Mode 0: Convert Image (Existing Single/Batch Image & PDF Conversion + SVG)
// ============================================================================

void MainPage::executeConvertImage() {
  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No Files Selected",
                    "Please select at least one file.",
                    ui::ToastVariant::Warning);
    return;
  }

  m_currentController = nullptr;
  DropFileWidget::ImageExtension targetExt = m_targetImageExtension;
  QString targetFormatString = m_dragWidget->imageExtensionToString(targetExt);
  int quality = m_qualitySlider->getValue();
  using FileStatus = DropFileWidget::FileStatus;
  const auto updateFileStatus = [this](const QString &path, FileStatus status,
                                       const QString &detail) {
    QMetaObject::invokeMethod(
        this,
        [this, path, status, detail]() {
          m_dragWidget->setFileStatus(path, status, detail);
        },
        Qt::QueuedConnection);
  };
  const auto updateProgress = [this](int completed, int total,
                                     const QString &label) {
    QMetaObject::invokeMethod(
        this,
        [this, completed, total, label]() {
          m_progressBar->setRange(0, total);
          m_progressBar->setValue(completed);
          if (!label.isEmpty()) {
            m_progressLabel->setText(label);
          }
        },
        Qt::QueuedConnection);
  };

  // SCENARIO 1: Images -> PDF (Single or Merged Multi-Page)
  if (targetExt == DropFileWidget::ImageExtension::PDF) {
    QString suggestedName;
    if (sourcePaths.size() == 1) {
      suggestedName =
          QFileInfo(sourcePaths.first())
              .dir()
              .filePath(QFileInfo(sourcePaths.first()).baseName() + ".pdf");
    } else {
      suggestedName =
          QFileInfo(sourcePaths.first()).dir().filePath("merged_images.pdf");
    }

    QString outputPath = QFileDialog::getSaveFileName(
        this, sourcePaths.size() == 1 ? "Save PDF" : "Save Merged PDF",
        suggestedName, "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return;
    }

    m_isCancelled = false;
    setProcessingState(true);
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(totalFiles == 1);
    m_progressLabel->setText(QString("Adding %1 to PDF (1 of %2)...")
                                 .arg(QFileInfo(sourcePaths.first()).fileName())
                                 .arg(totalFiles));

    QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputPath,
                                              quality, totalFiles,
                                              updateFileStatus,
                                              updateProgress]() {
      QString tempOutputPath = outputPath + ".tmp";
      int clampedQuality = qBound(1, quality, 100);
      int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
      qreal scaleFactor = 0.35 + (0.65 * clampedQuality) / 100.0;

      QPdfWriter pdfWriter(tempOutputPath);
      pdfWriter.setResolution(targetDpi);
      pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

      qint64 totalOrigBytes = 0;
      int successCount = 0;
      int skippedCount = 0;
      int startedFiles = 0;
      QStringList includedPaths;
      QString failedPath;
      bool ok = true;

      QPainter painter;
      for (int i = 0; i < totalFiles; ++i) {
        if (m_isCancelled) {
          ok = false;
          break;
        }

        const QString &path = sourcePaths.at(i);
        startedFiles++;
        updateFileStatus(path, FileStatus::Processing, "Adding image to PDF");
        updateProgress(i, totalFiles,
                       QString("Adding %1 to PDF (%2 of %3)...")
                           .arg(QFileInfo(path).fileName())
                           .arg(i + 1)
                           .arg(totalFiles));
        QImage origImage = loadRasterOrSvgImage(path, true);
        if (origImage.isNull()) {
          skippedCount++;
          updateFileStatus(path, FileStatus::Failed, "Could not load image");
          updateProgress(i + 1, totalFiles, QString());
          continue;
        }

        // Preserve consistent physical page dimensions in points (at 96 base
        // DPI)
        QSizeF ptSize(origImage.width() * 72.0 / 96.0,
                      origImage.height() * 72.0 / 96.0);
        pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

        // Downscale & compress image according to quality slider
        QImage processedImg = origImage.convertToFormat(QImage::Format_RGB888);
        if (clampedQuality < 95) {
          QSize scaledSize(qMax(1, qRound(origImage.width() * scaleFactor)),
                           qMax(1, qRound(origImage.height() * scaleFactor)));
          processedImg = processedImg.scaled(scaledSize, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
          QByteArray jpegBytes;
          QBuffer buffer(&jpegBytes);
          if (buffer.open(QIODevice::WriteOnly)) {
            processedImg.save(&buffer, "JPG", clampedQuality);
            buffer.close();
            QImage reloaded;
            if (reloaded.loadFromData(jpegBytes, "JPG")) {
              processedImg = reloaded;
            }
          }
        }

        if (successCount == 0) {
          if (!painter.begin(&pdfWriter)) {
            ok = false;
            failedPath = path;
            break;
          }
          painter.setRenderHint(QPainter::SmoothPixmapTransform);
          painter.setRenderHint(QPainter::Antialiasing);
        } else {
          if (!pdfWriter.newPage()) {
            ok = false;
            failedPath = path;
            break;
          }
        }

        QRect pageRect = pdfWriter.pageLayout().paintRectPixels(targetDpi);
        painter.drawImage(pageRect, processedImg);

        totalOrigBytes += QFileInfo(path).size();
        successCount++;
        includedPaths.append(path);
        updateFileStatus(path, FileStatus::Waiting, "Added; awaiting PDF save");
        updateProgress(i + 1, totalFiles, QString());
      }

      if (ok && !m_isCancelled && successCount > 0) {
        updateProgress(startedFiles, totalFiles,
                       QString("Saving %1 (%2 image(s))...")
                           .arg(QFileInfo(outputPath).fileName())
                           .arg(successCount));
      }
      if (painter.isActive() && !painter.end()) {
        ok = false;
      }

      const bool cancelled = m_isCancelled;
      bool saved = false;
      if (!ok || cancelled || successCount == 0) {
        QFile::remove(tempOutputPath);
      } else {
        const bool hadExisting = QFile::exists(outputPath);
        const QString backupPath = outputPath + ".bak";
        if (hadExisting) {
          QFile::remove(backupPath);
          if (!QFile::rename(outputPath, backupPath)) {
            QFile::remove(tempOutputPath);
            ok = false;
          }
        }
        if (ok && !QFile::rename(tempOutputPath, outputPath)) {
          QFile::remove(tempOutputPath);
          if (hadExisting) {
            QFile::rename(backupPath, outputPath);
          }
          ok = false;
        }
        if (ok && hadExisting) {
          QFile::remove(backupPath);
        }
        saved = ok;
      }

      QMetaObject::invokeMethod(
          this,
          [this, sourcePaths, outputPath, saved, cancelled, successCount,
           skippedCount, totalOrigBytes, includedPaths, failedPath,
           startedFiles]() {
            for (const QString &path : includedPaths) {
              m_dragWidget->setFileStatus(
                  path,
                  saved ? FileStatus::Succeeded
                        : (cancelled ? FileStatus::Cancelled
                                     : FileStatus::Failed),
                  saved ? QString("Saved in %1")
                              .arg(QFileInfo(outputPath).fileName())
                        : (cancelled ? "PDF creation cancelled; no output saved"
                                     : "Failed to save PDF"));
            }
            if (!failedPath.isEmpty()) {
              m_dragWidget->setFileStatus(failedPath, FileStatus::Failed,
                                          "Failed to add image to PDF");
            }
            for (int i = startedFiles; i < sourcePaths.size(); ++i) {
              m_dragWidget->setFileStatus(
                  sourcePaths.at(i),
                  cancelled ? FileStatus::Cancelled : FileStatus::Failed,
                  cancelled ? "Not processed (cancelled)"
                            : "Not processed: PDF creation failed");
            }
            setProcessingState(false);
            m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

            if (saved) {
              qint64 outSize = QFileInfo(outputPath).size();
              QString sizeStats =
                  QString("\n\nTotal Original: %1\nPDF Size: %2")
                      .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                      .arg(DropFileWidget::formatFileSize(outSize));
              QString msg =
                  (successCount == 1)
                      ? QString("Image converted to PDF successfully!%1")
                            .arg(sizeStats)
                      : QString("Successfully merged %1 images into PDF!%2")
                            .arg(successCount)
                            .arg(sizeStats);
              if (skippedCount > 0) {
                msg += QString("\n(%1 invalid image(s) skipped)")
                           .arg(skippedCount);
              }
              ui::Dialog::info(this, "Success", msg);
            } else if (cancelled) {
              ui::Toast::show(this, "Cancelled", "PDF creation was cancelled.",
                              ui::ToastVariant::Default);
            } else {
              ui::Toast::show(this, "Error", "Failed to create PDF!",
                              ui::ToastVariant::Destructive);
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 2: Input contains PDF -> Extract PDF Pages into Images
  bool hasPdfInput = false;
  for (const QString &p : sourcePaths) {
    if (QFileInfo(p).suffix().toLower() == "pdf") {
      hasPdfInput = true;
      break;
    }
  }

  if (hasPdfInput) {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory for Extracted Images", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    m_isCancelled = false;
    setProcessingState(true);
    m_progressBar->setRange(0, sourcePaths.size());
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(sourcePaths.size() == 1);
    m_progressLabel->setText(QString("Processing %1 (file 1 of %2)...")
                                 .arg(QFileInfo(sourcePaths.first()).fileName())
                                 .arg(sourcePaths.size()));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                              quality, targetExt, dragWidget,
                                              updateFileStatus,
                                              updateProgress]() {
      int totalExtracted = 0;
      int failedCount = 0;
      int completedFiles = 0;
      int startedFiles = 0;
      const int totalFiles = sourcePaths.size();
      qint64 totalOutputBytes = 0;
      QSet<QString> usedBaseNames;
      QString ext = dragWidget->imageExtensionToString(targetExt).toLower();
      const bool forceOpaque =
          (targetExt == DropFileWidget::JPG ||
           targetExt == DropFileWidget::JPEG ||
           targetExt == DropFileWidget::BMP);

      for (int i = 0; i < sourcePaths.size(); ++i) {
        if (m_isCancelled)
          break;

        const QString &sourcePath = sourcePaths.at(i);
        startedFiles++;
        updateFileStatus(sourcePath, FileStatus::Processing, "Loading source");
        updateProgress(completedFiles, totalFiles,
                       QString("Processing %1 (file %2 of %3)...")
                           .arg(QFileInfo(sourcePath).fileName())
                           .arg(i + 1)
                           .arg(sourcePaths.size()));
        if (QFileInfo(sourcePath).suffix().toLower() == "pdf") {
          QPdfDocument doc;
          if (doc.load(sourcePath) != QPdfDocument::Error::None) {
            failedCount++;
            completedFiles++;
            updateFileStatus(sourcePath, FileStatus::Failed,
                             "Could not load PDF");
            if (totalFiles > 1) {
              updateProgress(completedFiles, totalFiles, QString());
            }
            continue;
          }

          int pageCount = doc.pageCount();
          if (pageCount <= 0) {
            failedCount++;
            completedFiles++;
            updateFileStatus(sourcePath, FileStatus::Failed,
                             "PDF contains no pages");
            if (totalFiles > 1) {
              updateProgress(completedFiles, totalFiles, QString());
            }
            continue;
          }
          if (totalFiles == 1) {
            QMetaObject::invokeMethod(
                this,
                [this, pageCount]() {
                  m_progressBar->setIndeterminate(false);
                  m_progressBar->setRange(0, pageCount);
                  m_progressBar->setValue(0, false);
                },
                Qt::QueuedConnection);
          }
          const auto updatePageProgress =
              [totalFiles, pageCount, &completedFiles,
               updateProgress](int completedPages, const QString &label) {
                updateProgress(totalFiles == 1 ? completedPages
                                               : completedFiles,
                               totalFiles == 1 ? pageCount : totalFiles, label);
              };
          int completedPages = 0;
          int extractedPages = 0;
          int failedPages = 0;
          QString baseName = QFileInfo(sourcePath).baseName();

          for (int p = 0; p < pageCount; ++p) {
            if (m_isCancelled)
              break;

            updateFileStatus(
                sourcePath, FileStatus::Processing,
                QString("Page %1 of %2").arg(p + 1).arg(pageCount));
            updatePageProgress(
                completedPages,
                QString("Extracting %1 (file %2 of %3, page %4 of %5)...")
                    .arg(QFileInfo(sourcePath).fileName())
                    .arg(i + 1)
                    .arg(sourcePaths.size())
                    .arg(p + 1)
                    .arg(pageCount));
            QSizeF ptSize = doc.pagePointSize(p);
            int extractDpi =
                (quality <= 0)
                    ? 72
                    : (72 + static_cast<int>((quality / 100.0) * (600 - 72)));
            qreal scale = extractDpi / 72.0;
            QSize renderSize(
                qMax(1, static_cast<int>(ptSize.width() * scale)),
                qMax(1, static_cast<int>(ptSize.height() * scale)));
            QImage pageImg = doc.render(p, renderSize);

            if (pageImg.isNull()) {
              failedCount++;
              failedPages++;
              completedPages++;
              updatePageProgress(completedPages, QString());
              continue;
            }

            if (pageImg.hasAlphaChannel()) {
              QImage opaquePage(pageImg.size(), QImage::Format_RGB32);
              if (opaquePage.isNull()) {
                failedCount++;
                failedPages++;
                completedPages++;
                updatePageProgress(completedPages, QString());
                continue;
              }
              opaquePage.fill(Qt::white);
              opaquePage.setDotsPerMeterX(pageImg.dotsPerMeterX());
              opaquePage.setDotsPerMeterY(pageImg.dotsPerMeterY());
              {
                QPainter painter(&opaquePage);
                painter.drawImage(0, 0, pageImg);
              }
              pageImg = opaquePage;
            }

            QString uniqueName = QString("%1_page%2").arg(baseName).arg(p + 1);
            int suffix = 1;
            while (usedBaseNames.contains(uniqueName.toLower()) ||
                   QFile::exists(
                       QDir(outputDir).filePath(uniqueName + "." + ext))) {
              uniqueName = QString("%1_page%2_%3")
                               .arg(baseName)
                               .arg(p + 1)
                               .arg(suffix++);
            }
            usedBaseNames.insert(uniqueName.toLower());

            if (m_isCancelled)
              break;
            QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
            if (dragWidget->saveImage(&pageImg, baseOutputPath, quality,
                                      &targetExt)) {
              totalExtracted++;
              extractedPages++;
              totalOutputBytes += QFileInfo(baseOutputPath + "." + ext).size();
            } else {
              failedCount++;
              failedPages++;
            }

            completedPages++;
            updatePageProgress(completedPages, QString());
          }

          if (extractedPages + failedPages < pageCount) {
            updateFileStatus(
                sourcePath, FileStatus::Cancelled,
                QString("Cancelled: %1 of %2 pages saved; %3 failed")
                    .arg(extractedPages)
                    .arg(pageCount)
                    .arg(failedPages));
            break;
          } else if (failedPages > 0) {
            updateFileStatus(sourcePath, FileStatus::Failed,
                             QString("%1 of %2 pages saved; %3 failed")
                                 .arg(extractedPages)
                                 .arg(pageCount)
                                 .arg(failedPages));
          } else {
            updateFileStatus(sourcePath, FileStatus::Succeeded,
                             QString("Saved %1 page(s)").arg(extractedPages));
          }
        } else {
          QImage image = loadRasterOrSvgImage(sourcePath, forceOpaque);
          FileStatus status = FileStatus::Failed;
          QString detail = "Could not load image";
          if (image.isNull()) {
            failedCount++;
          } else {
            QString baseName = QFileInfo(sourcePath).baseName();
            QString uniqueName = baseName;
            int suffix = 1;
            while (usedBaseNames.contains(uniqueName.toLower()) ||
                   QFile::exists(
                       QDir(outputDir).filePath(uniqueName + "." + ext))) {
              uniqueName = QString("%1_%2").arg(baseName).arg(suffix++);
            }
            usedBaseNames.insert(uniqueName.toLower());

            QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
            if (m_isCancelled) {
              status = FileStatus::Cancelled;
              detail = "Cancelled before save";
            } else {
              if (dragWidget->saveImage(&image, baseOutputPath, quality,
                                        &targetExt)) {
                totalExtracted++;
                totalOutputBytes +=
                    QFileInfo(baseOutputPath + "." + ext).size();
                status = FileStatus::Succeeded;
                detail = QString("Saved as %1.%2").arg(uniqueName).arg(ext);
              } else {
                failedCount++;
                detail = QString("Failed to save as %1").arg(ext.toUpper());
              }
            }
          }
          updateFileStatus(sourcePath, status, detail);
          if (status == FileStatus::Cancelled) {
            break;
          }
        }
        completedFiles++;
        if (totalFiles > 1) {
          updateProgress(completedFiles, totalFiles, QString());
        }
      }

      const bool cancelled = m_isCancelled && completedFiles < totalFiles;
      QMetaObject::invokeMethod(
          this,
          [this, sourcePaths, startedFiles, cancelled, totalExtracted,
           failedCount, totalOutputBytes, outputDir]() {
            for (int i = startedFiles; i < sourcePaths.size(); ++i) {
              m_dragWidget->setFileStatus(sourcePaths.at(i),
                                          FileStatus::Cancelled,
                                          "Not processed (cancelled)");
            }
            setProcessingState(false);
            m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

            if (cancelled) {
              ui::Toast::show(this, "Cancelled", "Extraction was cancelled.",
                              ui::ToastVariant::Default);
            } else if (totalExtracted > 0) {
              QString msg =
                  QString("Extracted %1 image(s) to:\n%2\n\nTotal Size: %3")
                      .arg(totalExtracted)
                      .arg(outputDir)
                      .arg(DropFileWidget::formatFileSize(totalOutputBytes));
              if (failedCount > 0) {
                msg += QString("\n(%1 failed)").arg(failedCount);
              }
              ui::Dialog::info(this, "Success", msg);
            } else {
              ui::Toast::show(this, "Error",
                              "Failed to extract images from PDF!",
                              ui::ToastVariant::Destructive);
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 3: Standard Image / SVG -> Image Conversion (Single & Batch)
  const bool forceOpaque =
      (targetExt == DropFileWidget::JPG ||
       targetExt == DropFileWidget::JPEG ||
       targetExt == DropFileWidget::BMP);

  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
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

    const QString targetSuffix = targetFormatString.toLower();
    if (QFileInfo(outputPathWithExt).suffix().toLower() != targetSuffix) {
      outputPathWithExt += "." + targetSuffix;
    }

    QFileInfo outFi(outputPathWithExt);
    QString outputPathWithoutExt =
        outFi.dir().filePath(outFi.completeBaseName());

    m_isCancelled = false;
    setProcessingState(true);
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(true);
    m_progressLabel->setText(QString("Converting %1 (1 of 1)...")
                                 .arg(QFileInfo(sourcePath).fileName()));
    m_dragWidget->setFileStatus(sourcePath, FileStatus::Processing,
                                "Converting image");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePath, outputPathWithoutExt, outputPathWithExt, quality,
         targetExt, targetFormatString, dragWidget, forceOpaque]() {
          QImage image = loadRasterOrSvgImage(sourcePath, forceOpaque);
          FileStatus status = FileStatus::Failed;
          QString detail;
          if (image.isNull()) {
            detail = "Could not load image";
          } else if (m_isCancelled) {
            status = FileStatus::Cancelled;
            detail = "Cancelled before save";
          } else if (dragWidget->saveImage(&image, outputPathWithoutExt,
                                           quality, &targetExt)) {
            status = FileStatus::Succeeded;
            detail = QString("Saved as %1")
                         .arg(QFileInfo(outputPathWithExt).fileName());
          } else {
            detail =
                QString("Failed to save image as %1").arg(targetFormatString);
          }

          QMetaObject::invokeMethod(
              this,
              [this, sourcePath, status, detail, outputPathWithExt]() {
                m_dragWidget->setFileStatus(sourcePath, status, detail);
                m_progressBar->setValue(
                    status == FileStatus::Cancelled ? 0 : 1, false);
                setProcessingState(false);
                m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

                if (status == FileStatus::Succeeded) {
                  qint64 origSize = QFileInfo(sourcePath).size();
                  qint64 outSize = QFileInfo(outputPathWithExt).size();
                  QString sizeStats;
                  if (origSize > 0 && outSize > 0) {
                    double diffPct =
                        (1.0 - (static_cast<double>(outSize) / origSize)) *
                        100.0;
                    sizeStats =
                        QString("\n\nOriginal: %1\nConverted: %2 (%3% %4)")
                            .arg(DropFileWidget::formatFileSize(origSize))
                            .arg(DropFileWidget::formatFileSize(outSize))
                            .arg(qAbs(diffPct), 0, 'f', 1)
                            .arg(diffPct >= 0 ? "smaller" : "larger");
                  }

                  ui::Dialog::info(
                      this, "Success",
                      QString("Image converted successfully to %1%2")
                          .arg(outputPathWithExt)
                          .arg(sizeStats));
                } else if (status == FileStatus::Cancelled) {
                  ui::Toast::show(this, "Cancelled",
                                  "Image conversion was cancelled.",
                                  ui::ToastVariant::Default);
                } else {
                  ui::Toast::show(this, "Error", detail,
                                  ui::ToastVariant::Destructive);
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  } else {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    m_isCancelled = false;
    setProcessingState(true);
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(false);
    m_progressLabel->setText(QString("Converting %1 (1 of %2)...")
                                 .arg(QFileInfo(sourcePaths.first()).fileName())
                                 .arg(totalFiles));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePaths, outputDir, quality, targetExt, totalFiles,
         dragWidget, updateFileStatus, updateProgress, forceOpaque]() {
          int successCount = 0;
          int failureCount = 0;
          int completedFiles = 0;
          qint64 totalOrigBytes = 0;
          qint64 totalOutputBytes = 0;
          QSet<QString> usedBaseNames;
          QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

          for (int i = 0; i < totalFiles; ++i) {
            if (m_isCancelled) {
              break;
            }

            const QString &sourcePath = sourcePaths.at(i);
            updateFileStatus(sourcePath, FileStatus::Processing,
                             "Converting image");
            updateProgress(completedFiles, totalFiles,
                           QString("Converting %1 (%2 of %3)...")
                               .arg(QFileInfo(sourcePath).fileName())
                               .arg(i + 1)
                               .arg(totalFiles));
            QImage image = loadRasterOrSvgImage(sourcePath, forceOpaque);
            FileStatus status = FileStatus::Failed;
            QString detail = "Could not load image";
            if (image.isNull()) {
              failureCount++;
            } else {
              QFileInfo fileInfo(sourcePath);
              QString baseName = fileInfo.baseName();
              QString uniqueBaseName = baseName;
              int suffix = 1;
              while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
                     QFile::exists(
                         QDir(outputDir).filePath(uniqueBaseName + "." + ext))) {
                uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
              }
              usedBaseNames.insert(uniqueBaseName.toLower());

              if (m_isCancelled)
                break;
              QString baseOutputName = QDir(outputDir).filePath(uniqueBaseName);
              if (dragWidget->saveImage(&image, baseOutputName, quality,
                                        &targetExt)) {
                successCount++;
                totalOrigBytes += fileInfo.size();
                QString outputFilePath = baseOutputName + "." + ext;
                totalOutputBytes += QFileInfo(outputFilePath).size();
                status = FileStatus::Succeeded;
                detail = QString("Saved as %1.%2").arg(uniqueBaseName).arg(ext);
              } else {
                failureCount++;
                detail = QString("Failed to save as %1").arg(ext.toUpper());
              }
            }

            completedFiles++;
            updateFileStatus(sourcePath, status, detail);
            updateProgress(completedFiles, totalFiles, QString());
          }

          const bool cancelled = m_isCancelled && completedFiles < totalFiles;
          QMetaObject::invokeMethod(
              this,
              [this, sourcePaths, completedFiles, cancelled, successCount,
               failureCount, totalOrigBytes, totalOutputBytes]() {
                for (int i = completedFiles; i < sourcePaths.size(); ++i) {
                  m_dragWidget->setFileStatus(sourcePaths.at(i),
                                              FileStatus::Cancelled,
                                              "Not processed (cancelled)");
                }
                setProcessingState(false);
                m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

                QString title = cancelled ? "Batch Conversion Cancelled"
                                          : "Batch Conversion Complete";
                QString message =
                    QString("%1 image(s) converted successfully.\n%2 image(s) "
                            "failed.")
                        .arg(successCount)
                        .arg(failureCount);

                if (totalOrigBytes > 0 && totalOutputBytes > 0 &&
                    successCount > 0) {
                  double diffPct =
                      (1.0 - (static_cast<double>(totalOutputBytes) /
                              totalOrigBytes)) *
                      100.0;
                  message += QString("\n\nTotal Original: %1\nTotal Converted: "
                                     "%2 (%3% %4)")
                                 .arg(DropFileWidget::formatFileSize(
                                     totalOrigBytes))
                                 .arg(DropFileWidget::formatFileSize(
                                     totalOutputBytes))
                                 .arg(qAbs(diffPct), 0, 'f', 1)
                                 .arg(diffPct >= 0 ? "smaller" : "larger");
                }

                if (cancelled) {
                  message += "\n(Operation was stopped early by user)";
                  ui::Toast::show(this, "Cancelled", message,
                                  ui::ToastVariant::Default);
                } else {
                  ui::Dialog::info(this, title, message);
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  }
}

void MainPage::onImageTargetExtensionChanged() {
  int value =
      m_targetExtensionSelect ? m_targetExtensionSelect->currentIndex() : 0;
  if (value == 0) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 1) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPEG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 2) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PNG;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("PNG (Lossless - Quality N/A)");
  } else if (value == 3) {
    m_targetImageExtension = DropFileWidget::ImageExtension::WEBP;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 4) {
    m_targetImageExtension = DropFileWidget::ImageExtension::TIFF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 5) {
    m_targetImageExtension = DropFileWidget::ImageExtension::BMP;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("BMP (Uncompressed)");
  } else if (value == 6) {
    m_targetImageExtension = DropFileWidget::ImageExtension::GIF;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("GIF (Indexed Color)");
  } else if (value == 7) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PDF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("PDF Image Quality");
  }
}
