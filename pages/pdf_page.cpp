#include "pdf_page.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTransform>
#include <QtConcurrent/QtConcurrent>
#include <memory>

namespace {
bool isSamePdf(const QFileInfo &source, const QFileInfo &destination) {
  const QString canonicalSource = source.canonicalFilePath();
  const QString canonicalDestination = destination.canonicalFilePath();
#ifdef Q_OS_WIN
  constexpr Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
  constexpr Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
  return source == destination ||
         (!canonicalSource.isEmpty() && !canonicalDestination.isEmpty() &&
          canonicalSource.compare(canonicalDestination, pathCase) == 0);
}

bool savePdfAtomically(const QString &candidatePath, const QString &outputPath,
                       const std::atomic<bool> &cancelled, QString &error) {
  QFile candidate(candidatePath);
  QSaveFile destination(outputPath);
  // Never fall back to truncating an existing destination on a failed save.
  destination.setDirectWriteFallback(false);
  if (!candidate.open(QIODevice::ReadOnly)) {
    error = QString("Cannot read the generated PDF: %1")
                .arg(candidate.errorString());
    return false;
  }
  if (!destination.open(QIODevice::WriteOnly)) {
    error = QString("Cannot save the output PDF: %1")
                .arg(destination.errorString());
    return false;
  }

  while (!candidate.atEnd()) {
    if (cancelled) {
      return false;
    }
    const QByteArray chunk = candidate.read(1024 * 1024);
    if (candidate.error() != QFileDevice::NoError) {
      error = QString("Cannot read the generated PDF: %1")
                  .arg(candidate.errorString());
      return false;
    }
    if (destination.write(chunk) != chunk.size()) {
      error = QString("Cannot save the output PDF: %1")
                  .arg(destination.errorString());
      return false;
    }
  }
  if (cancelled) {
    return false;
  }
  if (!destination.commit()) {
    error = QString("Cannot commit the output PDF: %1")
                .arg(destination.errorString());
    return false;
  }
  return true;
}

QImage renderPdfPageOnWhite(QPdfDocument &doc, int pageIndex,
                            const QSize &pixelSize,
                            QImage::Format format = QImage::Format_RGB32) {
  if (pixelSize.width() <= 0 || pixelSize.height() <= 0) {
    return QImage();
  }
  const QImage rendered = doc.render(pageIndex, pixelSize);
  if (rendered.isNull()) {
    return QImage();
  }
  QImage canvas(pixelSize, format);
  canvas.fill(Qt::white);
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.drawImage(0, 0, rendered);
  painter.end();
  return canvas;
}

bool runGhostscriptProcess(const QString &executable, const QStringList &args,
                           const QString &workingDir,
                           const QString &expectedOutputPath,
                           const std::atomic<bool> &cancelled, QString &error,
                           bool failOnWarnings = true) {
  QProcess proc;
  proc.setWorkingDirectory(workingDir);
  proc.setProcessChannelMode(QProcess::MergedChannels);
  QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
  environment.remove("GS_OPTIONS");
  proc.setProcessEnvironment(environment);
  proc.start(executable, args);
  if (!proc.waitForStarted(5000)) {
    error = QString("Cannot start Ghostscript: %1. Check the bundled runtime "
                    "and its DLLs/resources, or reinstall Ghostscript.")
                .arg(proc.errorString());
    return false;
  }

  QElapsedTimer timer;
  timer.start();
  QByteArray diagnostics;
  while (proc.state() != QProcess::NotRunning) {
    proc.waitForFinished(150);
    diagnostics = (diagnostics + proc.readAll()).right(16384);
    if (cancelled || timer.elapsed() > 15 * 60 * 1000) {
      proc.kill();
      proc.waitForFinished(3000);
      if (!cancelled) {
        error = "Ghostscript exceeded the 15-minute processing limit.";
      }
      return false;
    }
  }
  diagnostics = (diagnostics + proc.readAll()).right(16384);
  if (cancelled) {
    return false;
  }
  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
    error = QString("Ghostscript failed (exit %1): %2")
                .arg(proc.exitCode())
                .arg(QString::fromLocal8Bit(diagnostics).trimmed().right(4096));
    qWarning().noquote() << error;
    return false;
  }
  if (failOnWarnings && !diagnostics.trimmed().isEmpty()) {
    error = QString("Ghostscript reported warnings; the output was not saved "
                    "to avoid accepting missing or substituted content: %1")
                .arg(QString::fromLocal8Bit(diagnostics).trimmed().right(4096));
    qWarning().noquote() << error;
    return false;
  }
  if (QFileInfo(expectedOutputPath).size() <= 0) {
    error = "Ghostscript did not produce a nonempty PDF.";
    return false;
  }
  return true;
}
} // namespace

PdfPage::PdfPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)) {
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
  connect(m_toolTabs, &ui::Tabs::currentChanged, this,
          &PdfPage::onToolTabChanged);

  m_dragWidget = new DropFileWidget(this, "PDF");
  m_imageDragWidget = new DropFileWidget(this, "IMAGE");
  m_imageDragWidget->setVisible(false);

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
  mainLayout->addWidget(m_optionsStack);
  mainLayout->addWidget(m_qualityRowWidget);
  mainLayout->addWidget(m_progressLabel);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
  setLayout(mainLayout);

  updateToolModeUi();

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &PdfPage::applyThemeStyles);
  applyThemeStyles();
}

PdfPage::~PdfPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

DropFileWidget *PdfPage::activeDropWidget() const {
  return m_currentTool == PdfToolMode::ImagesToPdf ? m_imageDragWidget
                                                   : m_dragWidget;
}

void PdfPage::setupToolPanels() {
  // Page 0: Compress (uses m_qualityRowWidget)
  QWidget *compressDummy = new QWidget(m_optionsStack);
  m_optionsStack->addWidget(compressDummy);

  // Page 1: Merge PDF
  m_mergePanel = new QWidget(m_optionsStack);
  QVBoxLayout *mergeLayout = new QVBoxLayout(m_mergePanel);
  mergeLayout->setContentsMargins(0, 0, 0, 0);
  mergeLayout->setSpacing(6);

  QHBoxLayout *mergeHeaderLayout = new QHBoxLayout();
  mergeHeaderLayout->setContentsMargins(0, 0, 0, 0);
  mergeHeaderLayout->setSpacing(8);

  m_mergeHintLabel = new QLabel(
      "Merge Order (drag items or use buttons to reorder):", m_mergePanel);
  m_mergeSyncBtn =
      new ui::Button("Sync from Drop Zone", ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Small, m_mergePanel);
  m_mergeUpBtn = new ui::Button("Move Up", ui::ButtonVariant::Outline,
                                ui::ButtonSize::Small, m_mergePanel);
  m_mergeDownBtn = new ui::Button("Move Down", ui::ButtonVariant::Outline,
                                  ui::ButtonSize::Small, m_mergePanel);

  connect(m_mergeSyncBtn, &QPushButton::clicked, this,
          &PdfPage::syncMergeListFromDropWidget);
  connect(m_mergeUpBtn, &QPushButton::clicked, this, &PdfPage::moveMergeItemUp);
  connect(m_mergeDownBtn, &QPushButton::clicked, this,
          &PdfPage::moveMergeItemDown);

  mergeHeaderLayout->addWidget(m_mergeHintLabel, 1);
  mergeHeaderLayout->addWidget(m_mergeSyncBtn);
  mergeHeaderLayout->addWidget(m_mergeUpBtn);
  mergeHeaderLayout->addWidget(m_mergeDownBtn);

  m_mergeOrderList = new QListWidget(m_mergePanel);
  m_mergeOrderList->setDragDropMode(QAbstractItemView::InternalMove);
  m_mergeOrderList->setDefaultDropAction(Qt::MoveAction);
  m_mergeOrderList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_mergeOrderList->setFixedHeight(115);

  mergeLayout->addLayout(mergeHeaderLayout);
  mergeLayout->addWidget(m_mergeOrderList);
  m_optionsStack->addWidget(m_mergePanel);

  // Page 2: Split PDF
  m_splitPanel = new QWidget(m_optionsStack);
  QHBoxLayout *splitLayout = new QHBoxLayout(m_splitPanel);
  splitLayout->setContentsMargins(0, 0, 0, 0);
  splitLayout->setSpacing(12);

  QStringList splitModes = {"Custom Page Ranges (e.g. 1-3, 5, 8-12)",
                            "Fixed Interval (Every N Pages)",
                            "Extract All Pages (1 PDF per page)"};
  m_splitModeSelect = new ui::Select(splitModes, m_splitPanel);
  m_splitModeField =
      new ui::FormField("Split Mode", m_splitModeSelect, m_splitPanel);
  connect(m_splitModeSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &PdfPage::onSplitModeChanged);

  m_splitRangesInput = new ui::Input("e.g. 1-3, 5, 8-12", m_splitPanel);
  m_splitRangesInput->setText("1");
  m_splitRangesField = new ui::FormField("Page Ranges (1-based)",
                                         m_splitRangesInput, m_splitPanel);

  m_splitIntervalInput = new ui::NumberInput(m_splitPanel);
  m_splitIntervalInput->setRange(1, 1000);
  m_splitIntervalInput->setValue(1);
  m_splitIntervalInput->setSuffix(" page(s)");
  m_splitIntervalField =
      new ui::FormField("Interval Size", m_splitIntervalInput, m_splitPanel);

  splitLayout->addWidget(m_splitModeField, 2, Qt::AlignTop);
  splitLayout->addWidget(m_splitRangesField, 2, Qt::AlignTop);
  splitLayout->addWidget(m_splitIntervalField, 1, Qt::AlignTop);
  m_optionsStack->addWidget(m_splitPanel);
  onSplitModeChanged(0);

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
  }
  return "Cancel";
}

void PdfPage::onToolTabChanged(int index) {
  if (m_isProcessing) {
    return;
  }
  m_currentTool = static_cast<PdfToolMode>(
      qBound(0, index, static_cast<int>(PdfToolMode::Grayscale)));
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
  if (m_dragWidget) {
    m_dragWidget->setVisible(!isImagesInput);
  }
  if (m_imageDragWidget) {
    m_imageDragWidget->setVisible(isImagesInput);
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

void PdfPage::onSplitModeChanged(int index) {
  if (m_splitRangesInput) {
    m_splitRangesInput->setEnabled(index == 0 && !m_isProcessing);
  }
  if (m_splitIntervalInput) {
    m_splitIntervalInput->setEnabled(index == 1 && !m_isProcessing);
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
  for (QLabel *lbl :
       {m_mergeHintLabel, m_organizeHintLabel, m_imageOrderHintLabel}) {
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

  for (QListWidget *listWidget :
       {m_mergeOrderList, m_organizePageList, m_imageOrderList}) {
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
      drop->finishProcessing(m_isCancelled);
    }
  }
}

void PdfPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Cancelling...");
  }
}

void PdfPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  switch (m_currentTool) {
  case PdfToolMode::Compress: {
    const QStringList sourcePaths = m_dragWidget->getFilePaths();
    if (sourcePaths.isEmpty()) {
      ui::Toast::show(this, "No PDF Selected",
                      "Please select at least one PDF file.",
                      ui::ToastVariant::Warning);
      return;
    }
    const int compressionQuality = m_qualitySlider->getValue();
    if (sourcePaths.size() == 1) {
      compressSinglePdf(sourcePaths.first(), compressionQuality);
    } else {
      compressBatchPdf(sourcePaths, compressionQuality);
    }
    break;
  }
  case PdfToolMode::Merge:
    mergePdfs();
    break;
  case PdfToolMode::Split:
    splitPdfs();
    break;
  case PdfToolMode::Organize:
    organizePdf();
    break;
  case PdfToolMode::PdfToImages:
    exportPdfToImages();
    break;
  case PdfToolMode::ImagesToPdf:
    imagesToPdf();
    break;
  case PdfToolMode::Grayscale: {
    const QStringList sourcePaths = m_dragWidget->getFilePaths();
    if (sourcePaths.isEmpty()) {
      ui::Toast::show(this, "No PDF Selected",
                      "Please select at least one PDF file.",
                      ui::ToastVariant::Warning);
      return;
    }
    const int quality = m_qualitySlider->getValue();
    if (sourcePaths.size() == 1) {
      convertSinglePdfToGrayscale(sourcePaths.first(), quality);
    } else {
      convertBatchPdfToGrayscale(sourcePaths, quality);
    }
    break;
  }
  }
}

// ============================================================================
// Tool 0: Compress PDF (Existing)
// ============================================================================

void PdfPage::compressSinglePdf(const QString &sourcePath, int quality) {
  QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Compressed PDF",
      QFileInfo(sourcePath)
          .dir()
          .filePath(QFileInfo(sourcePath).baseName() + "_compressed.pdf"),
      "PDF Files (*.pdf)");

  if (outputPath.isEmpty()) {
    return;
  }

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setRange(0, 1);
  m_progressBar->setValue(0, false);
  m_progressBar->setIndeterminate(true);
  m_progressLabel->setText(QString("Compressing %1 (1 of 1)...")
                               .arg(QFileInfo(sourcePath).fileName()));
  m_dragWidget->setFileStatus(sourcePath,
                              DropFileWidget::FileStatus::Processing,
                              "Optimizing PDF with Ghostscript");

  QFuture<void> future = QtConcurrent::run([this, sourcePath, outputPath,
                                            quality]() {
    const CompressionResult result =
        compressPdf(sourcePath, outputPath, quality);

    QMetaObject::invokeMethod(
        this,
        [this, sourcePath, result, outputPath]() {
          using FileStatus = DropFileWidget::FileStatus;
          FileStatus status = FileStatus::Failed;
          QString detail = result.error;
          switch (result.status) {
          case CompressionStatus::Compressed:
            status = FileStatus::Succeeded;
            detail = QString("Saved to %1").arg(outputPath);
            break;
          case CompressionStatus::Unchanged:
            status = FileStatus::Unchanged;
            detail = "No smaller PDF produced; no output saved";
            break;
          case CompressionStatus::Cancelled:
            status = FileStatus::Cancelled;
            detail = "Compression cancelled; no output saved";
            break;
          case CompressionStatus::Failed:
            break;
          }
          m_dragWidget->setFileStatus(sourcePath, status, detail);
          if (result.status != CompressionStatus::Cancelled) {
            m_progressBar->setValue(1, false);
          }
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (result.status == CompressionStatus::Compressed) {
            const double savedPercent =
                100.0 * (1.0 - static_cast<double>(result.outputBytes) /
                                   result.originalBytes);
            ui::Dialog::info(
                this, "PDF Compressed",
                QString("Saved to: %1\n\nOriginal: %2\nCompressed: %3 "
                        "(%4% smaller)")
                    .arg(outputPath)
                    .arg(DropFileWidget::formatFileSize(result.originalBytes))
                    .arg(DropFileWidget::formatFileSize(result.outputBytes))
                    .arg(savedPercent, 0, 'f', 1));
          } else if (result.status == CompressionStatus::Unchanged) {
            ui::Dialog::info(
                this, "No Reduction Achieved",
                "These settings did not produce a smaller PDF. "
                "The original PDF and destination were left unchanged.");
          } else if (result.status == CompressionStatus::Cancelled) {
            ui::Toast::show(this, "Cancelled", "PDF compression was cancelled.",
                            ui::ToastVariant::Default);
          } else {
            ui::Toast::show(this, "PDF Compression Failed", result.error,
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

void PdfPage::compressBatchPdf(const QStringList &sourcePaths, int quality) {
  QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Compressed PDFs", QDir::homePath());

  if (outputDir.isEmpty()) {
    return;
  }

  m_isCancelled = false;
  setProcessingState(true);
  int totalFiles = sourcePaths.size();
  m_progressBar->setIndeterminate(false);
  m_progressBar->setRange(0, totalFiles);
  m_progressBar->setValue(0, false);
  m_progressLabel->setText(
      QString("Compressing 0 of %1 PDFs...").arg(totalFiles));

  QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                            quality, totalFiles]() {
    int successCount = 0;
    int unchangedCount = 0;
    int failureCount = 0;
    qint64 totalOrigBytes = 0;
    qint64 totalOutputBytes = 0;
    QStringList errors;
    QSet<QString> usedBaseNames;

    for (int i = 0; i < totalFiles; ++i) {
      if (m_isCancelled) {
        break;
      }

      const QString &sourcePath = sourcePaths.at(i);
      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, i, totalFiles]() {
            m_dragWidget->setFileStatus(sourcePath,
                                        DropFileWidget::FileStatus::Processing,
                                        "Optimizing PDF with Ghostscript");
            m_progressLabel->setText(
                QString("Compressing %1 (%2 of %3; %4 completed)...")
                    .arg(QFileInfo(sourcePath).fileName())
                    .arg(i + 1)
                    .arg(totalFiles)
                    .arg(i));
          },
          Qt::QueuedConnection);
      QFileInfo fileInfo(sourcePath);
      QString baseName = fileInfo.baseName() + "_compressed";
      QString uniqueBaseName = baseName;
      int suffix = 1;
      while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
             QFile::exists(QDir(outputDir).filePath(uniqueBaseName + ".pdf"))) {
        uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
      }
      usedBaseNames.insert(uniqueBaseName.toLower());

      QString outputPath = QDir(outputDir).filePath(uniqueBaseName + ".pdf");

      const CompressionResult result =
          compressPdf(sourcePath, outputPath, quality);
      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, outputPath, result, i, totalFiles]() {
            using FileStatus = DropFileWidget::FileStatus;
            FileStatus status = FileStatus::Failed;
            QString detail = result.error;
            switch (result.status) {
            case CompressionStatus::Compressed:
              status = FileStatus::Succeeded;
              detail = QString("Saved to %1").arg(outputPath);
              break;
            case CompressionStatus::Unchanged:
              status = FileStatus::Unchanged;
              detail = "No smaller PDF produced; no output saved";
              break;
            case CompressionStatus::Cancelled:
              status = FileStatus::Cancelled;
              detail = "Compression cancelled; no output saved";
              break;
            case CompressionStatus::Failed:
              break;
            }
            m_dragWidget->setFileStatus(sourcePath, status, detail);
            if (result.status != CompressionStatus::Cancelled) {
              m_progressBar->setValue(i + 1);
              m_progressLabel->setText(QString("Processed %1 of %2 PDFs")
                                           .arg(i + 1)
                                           .arg(totalFiles));
            }
          },
          Qt::QueuedConnection);
      if (result.status == CompressionStatus::Cancelled) {
        break;
      }
      if (result.status == CompressionStatus::Compressed) {
        successCount++;
        totalOrigBytes += result.originalBytes;
        totalOutputBytes += result.outputBytes;
      } else if (result.status == CompressionStatus::Unchanged) {
        unchangedCount++;
      } else {
        failureCount++;
        if (errors.size() < 3) {
          errors.append(
              QString("%1: %2").arg(fileInfo.fileName(), result.error));
        }
      }
    }

    QMetaObject::invokeMethod(
        this,
        [this, successCount, unchangedCount, failureCount, totalFiles,
         totalOrigBytes, totalOutputBytes, errors]() {
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          QString title = m_isCancelled ? "Batch Compression Cancelled"
                                        : "Batch Compression Complete";
          QString message =
              QString("%1 PDF(s) compressed.\n%2 PDF(s) unchanged (no output "
                      "saved).\n%3 PDF(s) failed.")
                  .arg(successCount)
                  .arg(unchangedCount)
                  .arg(failureCount);

          if (totalOrigBytes > 0 && totalOutputBytes > 0 && successCount > 0) {
            double diffPct = (1.0 - (static_cast<double>(totalOutputBytes) /
                                     totalOrigBytes)) *
                             100.0;
            message +=
                QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
                    .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                    .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                    .arg(qAbs(diffPct), 0, 'f', 1)
                    .arg(diffPct >= 0 ? "smaller" : "larger");
          }

          if (!errors.isEmpty()) {
            message += "\n\n" + errors.join("\n");
          }
          if (m_isCancelled) {
            message += QString("\n%1 PDF(s) not completed due to cancellation.")
                           .arg(totalFiles - successCount - unchangedCount -
                                failureCount);
          }
          ui::Dialog::info(this, title, message);
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

QString PdfPage::findGhostscriptExecutable() {
  // Single-file packagers can point to their extracted runtime explicitly.
  const QString configured = qEnvironmentVariable("IMAGECONVERTER_GHOSTSCRIPT");
  if (!configured.isEmpty()) {
    const QFileInfo executable(configured);
    return executable.isFile() && executable.isExecutable()
               ? executable.absoluteFilePath()
               : QString();
  }

  const QDir appDir(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
  const QStringList bundledNames = {"gs/bin/gswin64c.exe", "gswin64c.exe"};
  const QStringList systemNames = {"gswin64c", "gswin32c"};
#else
  const QStringList bundledNames = {"gs/bin/gs"};
  const QStringList systemNames = {"gs"};
#endif
  for (const QString &name : bundledNames) {
    const QFileInfo executable(appDir.filePath(name));
    if (executable.isFile() && executable.isExecutable()) {
      return executable.absoluteFilePath();
    }
  }
  for (const QString &name : systemNames) {
    const QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
      return found;
    }
  }
  return QString();
}

bool PdfPage::compressPdfWithGhostscript(const QString &executable,
                                         const QString &inputPath,
                                         const QString &tempOutputPath,
                                         int quality, QString &error,
                                         bool grayscale) {
  const int clampedQuality = qBound(0, quality, 100);
  const bool preserveImages = clampedQuality >= 80;
  QStringList args = {"-sDEVICE=pdfwrite",
                      "-dCompatibilityLevel=1.7",
                      "-dSAFER",
                      "-dPDFSTOPONERROR",
                      "-dNOPAUSE",
                      "-dQUIET",
                      "-dBATCH",
                      "-dAutoRotatePages=/None",
                      "-dDetectDuplicateImages=true",
                      "-dCompressFonts=true",
                      "-dCompressStreams=true",
                      "-dSubsetFonts=true",
                      "-dPreserveAnnots=true",
                      "-dPreserveMarkedContent=true",
                      "-dWantsOptionalContent=true",
                      "-dDownsampleMonoImages=false"};

  if (grayscale) {
    args << "-sColorConversionStrategy=Gray"
         << "-dProcessColorModel=/DeviceGray";
  } else {
    args << "-sColorConversionStrategy=LeaveColorUnchanged"
         << "-dPassThroughJPEGImages=true"
         << "-dPassThroughJPXImages=true";
  }

  if (preserveImages) {
    args << "-dDownsampleColorImages=false" << "-dDownsampleGrayImages=false"
         << "-dAutoFilterColorImages=false" << "-dAutoFilterGrayImages=false"
         << "-dColorImageFilter=/FlateEncode"
         << "-dGrayImageFilter=/FlateEncode";
  } else {
    const int targetDpi = 100 + (200 * clampedQuality) / 80;
    args << "-dDownsampleColorImages=true" << "-dDownsampleGrayImages=true"
         << "-dColorImageDownsampleType=/Bicubic"
         << "-dGrayImageDownsampleType=/Bicubic"
         << "-dColorImageDownsampleThreshold=1.5"
         << "-dGrayImageDownsampleThreshold=1.5"
         << QString("-dColorImageResolution=%1").arg(targetDpi)
         << QString("-dGrayImageResolution=%1").arg(targetDpi)
         << "-dAutoFilterColorImages=true" << "-dAutoFilterGrayImages=true";
  }

  // QFactor controls pdfwrite's JPEG encoder; -dJPEGQ is not its quality API.
  // Avoid chroma subsampling to keep colored edges and scanned text clearer.
  const QString imageParameters =
      QString("<< /QFactor %1 /Blend 1 /ColorTransform 1 "
              "/HSamples [1 1 1 1] /VSamples [1 1 1 1] >>")
          .arg(0.6 - 0.45 * clampedQuality / 80.0, 0, 'f', 3);
  const QString distillerParameters =
      preserveImages
          ? QString("<< /NeverEmbed [] >> setdistillerparams")
          : QString("<< /NeverEmbed [] /ColorImageDict %1 /GrayImageDict %1 "
                    "/ColorACSImageDict %1 /GrayACSImageDict %1 >> "
                    "setdistillerparams")
                .arg(imageParameters);
  // A fixed relative output name avoids Ghostscript's %d filename expansion
  // even when the user-selected destination directory contains percent signs.
  args << "-sOutputFile=candidate.pdf" << "-c" << distillerParameters << "-f"
       << QDir::toNativeSeparators(inputPath);

  return runGhostscriptProcess(executable, args,
                               QFileInfo(tempOutputPath).absolutePath(),
                               tempOutputPath, m_isCancelled, error, true);
}

PdfPage::CompressionResult PdfPage::compressPdf(const QString &inputPath,
                                                const QString &outputPath,
                                                int quality) {
  CompressionResult result;
  if (m_isCancelled) {
    result.status = CompressionStatus::Cancelled;
    return result;
  }

  const QFileInfo sourceInfo(inputPath);
  const QFileInfo destinationInfo(outputPath);
  result.originalBytes = sourceInfo.size();
  const QDateTime sourceModified = sourceInfo.lastModified();
  if (!sourceInfo.isFile() || result.originalBytes <= 0) {
    result.error = "The input PDF is missing or empty.";
    return result;
  }
  if (isSamePdf(sourceInfo, destinationInfo)) {
    result.error = "Choose a different output file. The original PDF must not "
                   "be overwritten.";
    return result;
  }

  const QString executable = findGhostscriptExecutable();
  if (executable.isEmpty()) {
    result.error = "Ghostscript is required for PDF compression. Install the "
                   "ghostscript package on Linux or include the complete "
                   "gs runtime beside the Windows application. For an "
                   "extracted runtime, set IMAGECONVERTER_GHOSTSCRIPT to its "
                   "executable's full path. No PDF was changed.";
    return result;
  }

  QPdfDocument sourceDocument;
  const auto sourceError = sourceDocument.load(sourceInfo.absoluteFilePath());
  if (sourceError != QPdfDocument::Error::None ||
      sourceDocument.pageCount() <= 0) {
    result.error = QString("Cannot open the input PDF (PDF error %1). "
                           "Check whether it is damaged or password-protected.")
                       .arg(static_cast<int>(sourceError));
    return result;
  }

  QTemporaryDir temporaryDirectory(
      destinationInfo.absoluteDir().filePath(".pdf-compress-XXXXXX"));
  if (!temporaryDirectory.isValid()) {
    result.error = QString("Cannot create temporary output in the destination "
                           "directory: %1")
                       .arg(temporaryDirectory.errorString());
    return result;
  }
  const QString candidatePath = temporaryDirectory.filePath("candidate.pdf");
  if (!compressPdfWithGhostscript(executable, sourceInfo.absoluteFilePath(),
                                  candidatePath, quality, result.error,
                                  false)) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
    }
    return result;
  }

  QPdfDocument candidateDocument;
  const auto candidateError = candidateDocument.load(candidatePath);
  if (candidateError != QPdfDocument::Error::None ||
      candidateDocument.pageCount() != sourceDocument.pageCount()) {
    result.error = "The compressed PDF failed validation (unreadable output "
                   "or changed page count). No destination was replaced.";
    return result;
  }
  for (int page = 0; page < sourceDocument.pageCount(); ++page) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
      return result;
    }
    const QSizeF before = sourceDocument.pagePointSize(page);
    const QSizeF after = candidateDocument.pagePointSize(page);
    if (qAbs(before.width() - after.width()) > 0.5 ||
        qAbs(before.height() - after.height()) > 0.5) {
      result.error = "The compressed PDF changed page dimensions. "
                     "No destination was replaced.";
      return result;
    }
  }
  candidateDocument.close();
  sourceDocument.close();

  if (m_isCancelled) {
    result.status = CompressionStatus::Cancelled;
    return result;
  }
  const QFileInfo currentSource(sourceInfo.absoluteFilePath());
  if (currentSource.size() != result.originalBytes ||
      currentSource.lastModified() != sourceModified) {
    result.error = "The input PDF changed during compression. Please retry.";
    return result;
  }
  const qint64 candidateBytes = QFileInfo(candidatePath).size();
  if (candidateBytes <= 0) {
    result.error = "The compressed PDF is empty. No destination was replaced.";
    return result;
  }
  if (candidateBytes >= result.originalBytes) {
    result.status = CompressionStatus::Unchanged;
    result.outputBytes = result.originalBytes;
    return result;
  }

  if (isSamePdf(currentSource, QFileInfo(destinationInfo.absoluteFilePath()))) {
    result.error = "The destination now points to the input PDF. "
                   "No destination was replaced.";
    return result;
  }
  if (!savePdfAtomically(candidatePath, destinationInfo.absoluteFilePath(),
                         m_isCancelled, result.error)) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
    }
    return result;
  }
  result.status = CompressionStatus::Compressed;
  result.outputBytes = candidateBytes;
  return result;
}

// ============================================================================
// Tool 1: Merge PDF (Multiple PDFs -> 1 Combined PDF)
// ============================================================================

void PdfPage::syncMergeListFromDropWidget() {
  if (!m_mergeOrderList || !m_dragWidget) {
    return;
  }
  const QStringList dropPaths = m_dragWidget->getFilePaths();
  QStringList ordered;
  for (int i = 0; i < m_mergeOrderList->count(); ++i) {
    const QString path =
        m_mergeOrderList->item(i)->data(Qt::UserRole).toString();
    if (dropPaths.contains(path) && !ordered.contains(path)) {
      ordered.append(path);
    }
  }
  for (const QString &path : dropPaths) {
    if (!ordered.contains(path)) {
      ordered.append(path);
    }
  }

  m_mergeOrderList->clear();
  for (const QString &path : ordered) {
    QFileInfo fi(path);
    QListWidgetItem *item =
        new QListWidgetItem(QString("%1 (%2)")
                                .arg(fi.fileName())
                                .arg(DropFileWidget::formatFileSize(fi.size())),
                            m_mergeOrderList);
    item->setData(Qt::UserRole, path);
    item->setToolTip(path);
  }
}

void PdfPage::moveMergeItemUp() {
  if (!m_mergeOrderList)
    return;
  const int row = m_mergeOrderList->currentRow();
  if (row > 0) {
    QListWidgetItem *item = m_mergeOrderList->takeItem(row);
    m_mergeOrderList->insertItem(row - 1, item);
    m_mergeOrderList->setCurrentRow(row - 1);
  }
}

void PdfPage::moveMergeItemDown() {
  if (!m_mergeOrderList)
    return;
  const int row = m_mergeOrderList->currentRow();
  if (row >= 0 && row < m_mergeOrderList->count() - 1) {
    QListWidgetItem *item = m_mergeOrderList->takeItem(row);
    m_mergeOrderList->insertItem(row + 1, item);
    m_mergeOrderList->setCurrentRow(row + 1);
  }
}

bool PdfPage::mergePdfsWithGhostscript(const QString &executable,
                                       const QStringList &inputPaths,
                                       const QString &tempOutputPath,
                                       QString &error) {
  QStringList args = {"-sDEVICE=pdfwrite",
                      "-dCompatibilityLevel=1.7",
                      "-dSAFER",
                      "-dPDFSTOPONERROR",
                      "-dNOPAUSE",
                      "-dQUIET",
                      "-dBATCH",
                      "-dAutoRotatePages=/None",
                      "-dCompressFonts=true",
                      "-dCompressStreams=true",
                      "-dSubsetFonts=true",
                      "-dPreserveAnnots=true",
                      "-sOutputFile=candidate.pdf",
                      "-f"};
  for (const QString &path : inputPaths) {
    args << QDir::toNativeSeparators(path);
  }
  return runGhostscriptProcess(executable, args,
                               QFileInfo(tempOutputPath).absolutePath(),
                               tempOutputPath, m_isCancelled, error, false);
}

bool PdfPage::mergePdfsWithQtPdf(const QStringList &inputPaths,
                                 const QString &tempOutputPath,
                                 QString &error) {
  QPdfWriter writer(tempOutputPath);
  writer.setResolution(300);
  std::unique_ptr<QPainter> painter;

  for (const QString &inputPath : inputPaths) {
    if (m_isCancelled) {
      return false;
    }
    QPdfDocument doc;
    const auto loadErr = doc.load(inputPath);
    if (loadErr != QPdfDocument::Error::None || doc.pageCount() <= 0) {
      error = QString("Cannot open '%1' for merging.")
                  .arg(QFileInfo(inputPath).fileName());
      return false;
    }

    for (int page = 0; page < doc.pageCount(); ++page) {
      if (m_isCancelled) {
        return false;
      }
      const QSizeF ptSize = doc.pagePointSize(page);
      QPageLayout layout(QPageSize(ptSize, QPageSize::Point),
                         QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                         QPageLayout::Point);
      writer.setPageLayout(layout);
      if (!painter) {
        painter = std::make_unique<QPainter>(&writer);
        if (!painter->isActive()) {
          error = "Failed to initialize PDF writer.";
          return false;
        }
      } else {
        writer.newPage();
      }

      const QSize renderSize(qMax(1, qRound(ptSize.width() * 300.0 / 72.0)),
                             qMax(1, qRound(ptSize.height() * 300.0 / 72.0)));
      const QImage pageImg = renderPdfPageOnWhite(doc, page, renderSize);
      if (pageImg.isNull()) {
        error = QString("Failed to render page %1 of '%2'.")
                    .arg(page + 1)
                    .arg(QFileInfo(inputPath).fileName());
        return false;
      }
      painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
      painter->drawImage(
          QRectF(0, 0, painter->device()->width(), painter->device()->height()),
          pageImg);
    }
  }

  if (painter) {
    painter->end();
  }
  return QFileInfo(tempOutputPath).size() > 0;
}

void PdfPage::mergePdfs() {
  syncMergeListFromDropWidget();

  QStringList orderedPaths;
  for (int i = 0; i < m_mergeOrderList->count(); ++i) {
    orderedPaths.append(
        m_mergeOrderList->item(i)->data(Qt::UserRole).toString());
  }

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
    if (isSamePdf(QFileInfo(srcPath), destInfo)) {
      ui::Toast::show(
          this, "Invalid Output File",
          "The merged PDF destination cannot overwrite one of the input PDFs.",
          ui::ToastVariant::Destructive);
      return;
    }
  }

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setRange(0, orderedPaths.size());
  m_progressBar->setValue(0, false);
  m_progressBar->setIndeterminate(true);
  m_progressLabel->setText(
      QString("Merging %1 PDF files...").arg(orderedPaths.size()));

  for (const QString &srcPath : orderedPaths) {
    m_dragWidget->setFileStatus(srcPath, DropFileWidget::FileStatus::Processing,
                                "Merging into combined PDF");
  }

  QFuture<void> future = QtConcurrent::run([this, orderedPaths, outputPath]() {
    QString error;
    int expectedTotalPages = 0;
    for (const QString &srcPath : orderedPaths) {
      if (m_isCancelled) {
        break;
      }
      const QFileInfo srcInfo(srcPath);
      if (!srcInfo.isFile() || srcInfo.size() <= 0) {
        error = QString("Input file '%1' is missing or empty.")
                    .arg(srcInfo.fileName());
        break;
      }
      QPdfDocument doc;
      const auto err = doc.load(srcInfo.absoluteFilePath());
      if (err != QPdfDocument::Error::None || doc.pageCount() <= 0) {
        error = QString("Cannot open '%1' (damaged or password-protected).")
                    .arg(srcInfo.fileName());
        break;
      }
      expectedTotalPages += doc.pageCount();
    }

    bool succeeded = false;
    qint64 outputBytes = 0;
    const QFileInfo destinationInfo(outputPath);

    if (error.isEmpty() && !m_isCancelled) {
      QTemporaryDir tempDir(
          destinationInfo.absoluteDir().filePath(".pdf-merge-XXXXXX"));
      if (!tempDir.isValid()) {
        error = QString("Cannot create temporary directory: %1")
                    .arg(tempDir.errorString());
      } else {
        const QString candidatePath = tempDir.filePath("candidate.pdf");
        const QString gsExec = findGhostscriptExecutable();
        bool generated = false;
        if (!gsExec.isEmpty()) {
          QString gsErr;
          generated = mergePdfsWithGhostscript(gsExec, orderedPaths,
                                               candidatePath, gsErr);
          if (generated) {
            QPdfDocument checkDoc;
            if (checkDoc.load(candidatePath) != QPdfDocument::Error::None ||
                checkDoc.pageCount() != expectedTotalPages) {
              generated = false;
              QFile::remove(candidatePath);
            }
          }
        }
        if (!generated && !m_isCancelled) {
          generated = mergePdfsWithQtPdf(orderedPaths, candidatePath, error);
        }

        if (generated && !m_isCancelled) {
          QPdfDocument candidateDoc;
          if (candidateDoc.load(candidatePath) != QPdfDocument::Error::None ||
              candidateDoc.pageCount() != expectedTotalPages) {
            error = "Merged PDF validation failed (unexpected page count).";
          } else {
            candidateDoc.close();
            if (savePdfAtomically(candidatePath,
                                  destinationInfo.absoluteFilePath(),
                                  m_isCancelled, error)) {
              succeeded = true;
              outputBytes =
                  QFileInfo(destinationInfo.absoluteFilePath()).size();
            }
          }
        }
      }
    }

    QMetaObject::invokeMethod(
        this,
        [this, orderedPaths, outputPath, succeeded, expectedTotalPages,
         outputBytes, error]() {
          using FileStatus = DropFileWidget::FileStatus;
          for (const QString &srcPath : orderedPaths) {
            if (m_isCancelled) {
              m_dragWidget->setFileStatus(srcPath, FileStatus::Cancelled,
                                          "Merge cancelled");
            } else if (succeeded) {
              m_dragWidget->setFileStatus(
                  srcPath, FileStatus::Succeeded,
                  QString("Merged into %1").arg(outputPath));
            } else {
              m_dragWidget->setFileStatus(srcPath, FileStatus::Failed, error);
            }
          }
          if (!m_isCancelled && succeeded) {
            m_progressBar->setIndeterminate(false);
            m_progressBar->setValue(orderedPaths.size(), false);
          }
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (m_isCancelled) {
            ui::Toast::show(this, "Cancelled", "PDF merge was cancelled.",
                            ui::ToastVariant::Default);
          } else if (succeeded) {
            ui::Dialog::info(
                this, "PDFs Merged",
                QString("Merged %1 PDFs (%2 total pages) into:\n%3\n\n"
                        "Output Size: %4")
                    .arg(orderedPaths.size())
                    .arg(expectedTotalPages)
                    .arg(outputPath)
                    .arg(DropFileWidget::formatFileSize(outputBytes)));
          } else {
            ui::Toast::show(this, "PDF Merge Failed", error,
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

// ============================================================================
// Tool 2: Split PDF (1 PDF -> Multiple PDFs)
// ============================================================================

bool PdfPage::parsePageRanges(const QString &rangeSpec, int pageCount,
                              QVector<QPair<int, int>> &outRanges,
                              QString &error) {
  outRanges.clear();
  const QString trimmed = rangeSpec.trimmed();
  if (trimmed.isEmpty()) {
    error = "Please enter at least one page or range (e.g. 1-3, 5, 8-12).";
    return false;
  }

  const QStringList tokens = trimmed.split(',', Qt::SkipEmptyParts);
  if (tokens.isEmpty()) {
    error = "Please enter at least one page or range (e.g. 1-3, 5, 8-12).";
    return false;
  }

  for (const QString &rawToken : tokens) {
    const QString token = rawToken.trimmed();
    if (token.isEmpty()) {
      continue;
    }
    if (token.contains('-')) {
      const QStringList parts = token.split('-');
      if (parts.size() != 2) {
        error = QString("Invalid range syntax '%1'.").arg(token);
        return false;
      }
      bool okStart = false;
      bool okEnd = false;
      const int start = parts[0].trimmed().toInt(&okStart);
      const int end = parts[1].trimmed().toInt(&okEnd);
      if (!okStart || !okEnd || start < 1 || end < start || end > pageCount) {
        error = QString("Page range '%1' is invalid (document has %2 page(s)).")
                    .arg(token)
                    .arg(pageCount);
        return false;
      }
      outRanges.append(qMakePair(start, end));
    } else {
      bool okPage = false;
      const int page = token.toInt(&okPage);
      if (!okPage || page < 1 || page > pageCount) {
        error =
            QString("Page number '%1' is invalid (document has %2 page(s)).")
                .arg(token)
                .arg(pageCount);
        return false;
      }
      outRanges.append(qMakePair(page, page));
    }
  }

  if (outRanges.isEmpty()) {
    error = "Please enter at least one valid page number.";
    return false;
  }
  return true;
}

bool PdfPage::extractPageRangeWithGhostscript(const QString &executable,
                                              const QString &inputPath,
                                              int firstPage, int lastPage,
                                              const QString &tempOutputPath,
                                              QString &error) {
  QStringList args = {"-sDEVICE=pdfwrite",
                      "-dCompatibilityLevel=1.7",
                      "-dSAFER",
                      "-dPDFSTOPONERROR",
                      "-dNOPAUSE",
                      "-dQUIET",
                      "-dBATCH",
                      "-dAutoRotatePages=/None",
                      QString("-dFirstPage=%1").arg(firstPage),
                      QString("-dLastPage=%1").arg(lastPage),
                      "-sOutputFile=candidate.pdf",
                      "-f",
                      QDir::toNativeSeparators(inputPath)};
  return runGhostscriptProcess(executable, args,
                               QFileInfo(tempOutputPath).absolutePath(),
                               tempOutputPath, m_isCancelled, error, false);
}

bool PdfPage::extractPageRangeWithQtPdf(QPdfDocument &sourceDoc, int firstPage,
                                        int lastPage,
                                        const QString &tempOutputPath,
                                        QString &error) {
  QPdfWriter writer(tempOutputPath);
  writer.setResolution(300);
  std::unique_ptr<QPainter> painter;

  for (int page = firstPage - 1; page <= lastPage - 1; ++page) {
    if (m_isCancelled) {
      return false;
    }
    const QSizeF ptSize = sourceDoc.pagePointSize(page);
    QPageLayout layout(QPageSize(ptSize, QPageSize::Point),
                       QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                       QPageLayout::Point);
    writer.setPageLayout(layout);
    if (!painter) {
      painter = std::make_unique<QPainter>(&writer);
      if (!painter->isActive()) {
        error = "Failed to initialize PDF writer.";
        return false;
      }
    } else {
      writer.newPage();
    }

    const QSize renderSize(qMax(1, qRound(ptSize.width() * 300.0 / 72.0)),
                           qMax(1, qRound(ptSize.height() * 300.0 / 72.0)));
    const QImage pageImg = renderPdfPageOnWhite(sourceDoc, page, renderSize);
    if (pageImg.isNull()) {
      error = QString("Failed to render page %1.").arg(page + 1);
      return false;
    }
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(
        QRectF(0, 0, painter->device()->width(), painter->device()->height()),
        pageImg);
  }

  if (painter) {
    painter->end();
  }
  return QFileInfo(tempOutputPath).size() > 0;
}

void PdfPage::splitPdfs() {
  const QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No PDF Selected",
                    "Please select at least one PDF file to split.",
                    ui::ToastVariant::Warning);
    return;
  }

  const int splitMode =
      m_splitModeSelect ? m_splitModeSelect->currentIndex() : 0;
  const QString rangeSpec =
      m_splitRangesInput ? m_splitRangesInput->text().trimmed() : QString();
  const int interval =
      m_splitIntervalInput ? qMax(1, m_splitIntervalInput->value()) : 1;

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
    if (!parsePageRanges(rangeSpec, checkDoc.pageCount(), testRanges,
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

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setRange(0, sourcePaths.size());
  m_progressBar->setValue(0, false);
  m_progressLabel->setText("Splitting PDF files...");

  QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                            splitMode, rangeSpec, interval]() {
    int totalFilesCreated = 0;
    int failedDocs = 0;
    QStringList errors;
    QSet<QString> usedFileNames;
    const QString gsExec = findGhostscriptExecutable();

    for (int i = 0; i < sourcePaths.size(); ++i) {
      if (m_isCancelled) {
        break;
      }
      const QString &sourcePath = sourcePaths.at(i);
      const QFileInfo sourceInfo(sourcePath);

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, i, total = sourcePaths.size()]() {
            m_dragWidget->setFileStatus(sourcePath,
                                        DropFileWidget::FileStatus::Processing,
                                        "Splitting PDF pages");
            m_progressLabel->setText(QString("Splitting %1 (%2 of %3)...")
                                         .arg(QFileInfo(sourcePath).fileName())
                                         .arg(i + 1)
                                         .arg(total));
          },
          Qt::QueuedConnection);

      QPdfDocument sourceDoc;
      const auto loadErr = sourceDoc.load(sourceInfo.absoluteFilePath());
      if (loadErr != QPdfDocument::Error::None || sourceDoc.pageCount() <= 0) {
        failedDocs++;
        const QString err = "Cannot open PDF (damaged or encrypted).";
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
        QMetaObject::invokeMethod(
            this,
            [this, sourcePath, err]() {
              m_dragWidget->setFileStatus(
                  sourcePath, DropFileWidget::FileStatus::Failed, err);
            },
            Qt::QueuedConnection);
        continue;
      }

      const int pageCount = sourceDoc.pageCount();
      QVector<QPair<int, int>> ranges;
      QString rangeErr;
      if (splitMode == 0) {
        if (!parsePageRanges(rangeSpec, pageCount, ranges, rangeErr)) {
          failedDocs++;
          errors.append(QString("%1: %2").arg(sourceInfo.fileName(), rangeErr));
          QMetaObject::invokeMethod(
              this,
              [this, sourcePath, rangeErr]() {
                m_dragWidget->setFileStatus(
                    sourcePath, DropFileWidget::FileStatus::Failed, rangeErr);
              },
              Qt::QueuedConnection);
          continue;
        }
      } else if (splitMode == 1) {
        for (int start = 1; start <= pageCount; start += interval) {
          const int end = qMin(pageCount, start + interval - 1);
          ranges.append(qMakePair(start, end));
        }
      } else {
        for (int p = 1; p <= pageCount; ++p) {
          ranges.append(qMakePair(p, p));
        }
      }

      int createdForThisPdf = 0;
      QString docError;
      for (const auto &range : ranges) {
        if (m_isCancelled) {
          break;
        }
        const int firstPage = range.first;
        const int lastPage = range.second;
        const int expectedPages = lastPage - firstPage + 1;

        const QString baseStem = (firstPage == lastPage)
                                     ? QString("%1_page_%2")
                                           .arg(sourceInfo.baseName())
                                           .arg(firstPage)
                                     : QString("%1_pages_%2-%3")
                                           .arg(sourceInfo.baseName())
                                           .arg(firstPage)
                                           .arg(lastPage);

        QString uniqueStem = baseStem;
        int suffix = 1;
        while (usedFileNames.contains(uniqueStem.toLower()) ||
               QFile::exists(QDir(outputDir).filePath(uniqueStem + ".pdf"))) {
          uniqueStem = QString("%1_%2").arg(baseStem).arg(suffix++);
        }
        usedFileNames.insert(uniqueStem.toLower());
        const QString outFilePath =
            QDir(outputDir).filePath(uniqueStem + ".pdf");

        QTemporaryDir tempDir(QDir(outputDir).filePath(".pdf-split-XXXXXX"));
        if (!tempDir.isValid()) {
          docError = tempDir.errorString();
          break;
        }
        const QString candidatePath = tempDir.filePath("candidate.pdf");
        bool extracted = false;
        if (!gsExec.isEmpty()) {
          QString gsErr;
          extracted = extractPageRangeWithGhostscript(
              gsExec, sourceInfo.absoluteFilePath(), firstPage, lastPage,
              candidatePath, gsErr);
          if (extracted) {
            QPdfDocument checkDoc;
            if (checkDoc.load(candidatePath) != QPdfDocument::Error::None ||
                checkDoc.pageCount() != expectedPages) {
              extracted = false;
              QFile::remove(candidatePath);
            }
          }
        }
        if (!extracted && !m_isCancelled) {
          extracted = extractPageRangeWithQtPdf(sourceDoc, firstPage, lastPage,
                                                candidatePath, docError);
        }
        if (!extracted || m_isCancelled) {
          if (docError.isEmpty()) {
            docError = "Failed to extract page range.";
          }
          break;
        }

        QPdfDocument verifyDoc;
        if (verifyDoc.load(candidatePath) != QPdfDocument::Error::None ||
            verifyDoc.pageCount() != expectedPages) {
          docError = "Extracted PDF failed validation.";
          break;
        }
        verifyDoc.close();

        if (!savePdfAtomically(candidatePath, outFilePath, m_isCancelled,
                               docError)) {
          break;
        }
        createdForThisPdf++;
        totalFilesCreated++;
      }

      sourceDoc.close();

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, createdForThisPdf, docError, outputDir, i]() {
            using FileStatus = DropFileWidget::FileStatus;
            if (m_isCancelled) {
              m_dragWidget->setFileStatus(sourcePath, FileStatus::Cancelled,
                                          "Split cancelled");
            } else if (docError.isEmpty() && createdForThisPdf > 0) {
              m_dragWidget->setFileStatus(
                  sourcePath, FileStatus::Succeeded,
                  QString("Created %1 PDF file(s) in %2")
                      .arg(createdForThisPdf)
                      .arg(outputDir));
            } else {
              m_dragWidget->setFileStatus(sourcePath, FileStatus::Failed,
                                          docError);
            }
            if (!m_isCancelled) {
              m_progressBar->setValue(i + 1);
            }
          },
          Qt::QueuedConnection);

      if (!docError.isEmpty() && !m_isCancelled) {
        failedDocs++;
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), docError));
      }
    }

    QMetaObject::invokeMethod(
        this,
        [this, totalFilesCreated, failedDocs, outputDir, errors]() {
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (m_isCancelled) {
            ui::Toast::show(this, "Cancelled", "PDF split was cancelled.",
                            ui::ToastVariant::Default);
          } else if (totalFilesCreated > 0 && failedDocs == 0) {
            ui::Dialog::info(this, "PDF Split Complete",
                             QString("Created %1 split PDF file(s) in:\n%2")
                                 .arg(totalFilesCreated)
                                 .arg(outputDir));
          } else if (totalFilesCreated > 0) {
            ui::Dialog::info(
                this, "PDF Split Finished with Warnings",
                QString("Created %1 split PDF file(s) in:\n%2\n\nErrors:\n%3")
                    .arg(totalFilesCreated)
                    .arg(outputDir)
                    .arg(errors.join("\n")));
          } else {
            ui::Toast::show(this, "PDF Split Failed",
                            errors.isEmpty() ? "Could not split PDF."
                                             : errors.join("\n"),
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
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
    const QImage thumb = renderPdfPageOnWhite(doc, i, thumbSize);

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

void PdfPage::organizePdf() {
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

  QVector<PageOp> pageOps;
  pageOps.reserve(m_organizePageList->count());
  for (int i = 0; i < m_organizePageList->count(); ++i) {
    QListWidgetItem *item = m_organizePageList->item(i);
    PageOp op;
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

  if (isSamePdf(sourceInfo, QFileInfo(outputPath))) {
    ui::Toast::show(this, "Invalid Output File",
                    "Choose a different output file. The original PDF must not "
                    "be overwritten.",
                    ui::ToastVariant::Destructive);
    return;
  }

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setIndeterminate(false);
  m_progressBar->setRange(0, pageOps.size());
  m_progressBar->setValue(0, false);
  m_progressLabel->setText(
      QString("Saving %1 organized page(s)...").arg(pageOps.size()));
  m_dragWidget->setFileStatus(sourcePath,
                              DropFileWidget::FileStatus::Processing,
                              "Writing organized PDF");

  QFuture<void> future = QtConcurrent::run([this, sourcePath, outputPath,
                                            pageOps]() {
    QString error;
    bool succeeded = false;
    qint64 outputBytes = 0;
    const QFileInfo destinationInfo(outputPath);

    QPdfDocument sourceDoc;
    const auto loadErr = sourceDoc.load(sourcePath);
    if (loadErr != QPdfDocument::Error::None || sourceDoc.pageCount() <= 0) {
      error = "Cannot open source PDF.";
    } else {
      QTemporaryDir tempDir(
          destinationInfo.absoluteDir().filePath(".pdf-organize-XXXXXX"));
      if (!tempDir.isValid()) {
        error = tempDir.errorString();
      } else {
        const QString candidatePath = tempDir.filePath("candidate.pdf");
        QPdfWriter writer(candidatePath);
        writer.setResolution(300);
        std::unique_ptr<QPainter> painter;

        for (int i = 0; i < pageOps.size(); ++i) {
          if (m_isCancelled) {
            break;
          }
          const PageOp &op = pageOps.at(i);
          if (op.originalPageIndex < 0 ||
              op.originalPageIndex >= sourceDoc.pageCount()) {
            error = "Page index out of bounds.";
            break;
          }

          const QSizeF origPtSize =
              sourceDoc.pagePointSize(op.originalPageIndex);
          const bool swapDims =
              (op.rotationDegrees == 90 || op.rotationDegrees == 270);
          const QSizeF effectivePtSize =
              swapDims ? QSizeF(origPtSize.height(), origPtSize.width())
                       : origPtSize;

          QPageLayout layout(QPageSize(effectivePtSize, QPageSize::Point),
                             QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                             QPageLayout::Point);
          writer.setPageLayout(layout);

          if (!painter) {
            painter = std::make_unique<QPainter>(&writer);
            if (!painter->isActive()) {
              error = "Failed to initialize PDF writer.";
              break;
            }
          } else {
            writer.newPage();
          }

          const QSize renderSize(
              qMax(1, qRound(origPtSize.width() * 300.0 / 72.0)),
              qMax(1, qRound(origPtSize.height() * 300.0 / 72.0)));
          QImage pageImg =
              renderPdfPageOnWhite(sourceDoc, op.originalPageIndex, renderSize);
          if (pageImg.isNull()) {
            error = QString("Failed to render page %1.")
                        .arg(op.originalPageIndex + 1);
            break;
          }
          if (op.rotationDegrees != 0) {
            pageImg =
                pageImg.transformed(QTransform().rotate(op.rotationDegrees),
                                    Qt::SmoothTransformation);
          }

          painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
          painter->drawImage(QRectF(0, 0, painter->device()->width(),
                                    painter->device()->height()),
                             pageImg);

          QMetaObject::invokeMethod(
              this,
              [this, done = i + 1, total = pageOps.size()]() {
                if (!m_isCancelled) {
                  m_progressBar->setValue(done);
                  m_progressLabel->setText(QString("Rendered page %1 of %2...")
                                               .arg(done)
                                               .arg(total));
                }
              },
              Qt::QueuedConnection);
        }

        if (painter) {
          painter->end();
        }

        if (error.isEmpty() && !m_isCancelled) {
          QPdfDocument verifyDoc;
          if (verifyDoc.load(candidatePath) != QPdfDocument::Error::None ||
              verifyDoc.pageCount() != pageOps.size()) {
            error = "Organized PDF failed validation.";
          } else {
            verifyDoc.close();
            if (savePdfAtomically(candidatePath,
                                  destinationInfo.absoluteFilePath(),
                                  m_isCancelled, error)) {
              succeeded = true;
              outputBytes =
                  QFileInfo(destinationInfo.absoluteFilePath()).size();
            }
          }
        }
      }
    }

    sourceDoc.close();

    QMetaObject::invokeMethod(
        this,
        [this, sourcePath, outputPath, succeeded, pageOps, outputBytes,
         error]() {
          using FileStatus = DropFileWidget::FileStatus;
          if (m_isCancelled) {
            m_dragWidget->setFileStatus(sourcePath, FileStatus::Cancelled,
                                        "Operation cancelled");
          } else if (succeeded) {
            m_dragWidget->setFileStatus(
                sourcePath, FileStatus::Succeeded,
                QString("Saved organized PDF to %1").arg(outputPath));
          } else {
            m_dragWidget->setFileStatus(sourcePath, FileStatus::Failed, error);
          }
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (m_isCancelled) {
            ui::Toast::show(this, "Cancelled",
                            "Page organization was cancelled.",
                            ui::ToastVariant::Default);
          } else if (succeeded) {
            ui::Dialog::info(
                this, "Organized PDF Saved",
                QString("Saved %1 page(s) to:\n%2\n\nOutput Size: %3")
                    .arg(pageOps.size())
                    .arg(outputPath)
                    .arg(DropFileWidget::formatFileSize(outputBytes)));
          } else {
            ui::Toast::show(this, "Failed to Save Organized PDF", error,
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

// ============================================================================
// Tool 4: PDF to Images (PDF -> PNG, JPG, WEBP, TIFF)
// ============================================================================

void PdfPage::exportPdfToImages() {
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
    if (!parsePageRanges(rangeSpec, checkDoc.pageCount(), testRanges,
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

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setIndeterminate(false);
  m_progressBar->setRange(0, sourcePaths.size());
  m_progressBar->setValue(0, false);
  m_progressLabel->setText("Exporting PDF pages to images...");

  QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                            format, effectiveQuality, dpi,
                                            rangeSpec]() {
    int totalExportedImages = 0;
    int failedDocs = 0;
    QStringList errors;
    QSet<QString> usedFileNames;

    for (int i = 0; i < sourcePaths.size(); ++i) {
      if (m_isCancelled) {
        break;
      }
      const QString &sourcePath = sourcePaths.at(i);
      const QFileInfo sourceInfo(sourcePath);

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, i, total = sourcePaths.size(), dpi, format]() {
            m_dragWidget->setFileStatus(
                sourcePath, DropFileWidget::FileStatus::Processing,
                QString("Rendering pages to %1 at %2 DPI")
                    .arg(format.toUpper())
                    .arg(dpi));
            m_progressLabel->setText(QString("Exporting %1 (%2 of %3)...")
                                         .arg(QFileInfo(sourcePath).fileName())
                                         .arg(i + 1)
                                         .arg(total));
          },
          Qt::QueuedConnection);

      QPdfDocument doc;
      if (doc.load(sourceInfo.absoluteFilePath()) !=
              QPdfDocument::Error::None ||
          doc.pageCount() <= 0) {
        failedDocs++;
        const QString err = "Cannot open PDF (damaged or encrypted).";
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), err));
        QMetaObject::invokeMethod(
            this,
            [this, sourcePath, err]() {
              m_dragWidget->setFileStatus(
                  sourcePath, DropFileWidget::FileStatus::Failed, err);
            },
            Qt::QueuedConnection);
        continue;
      }

      QVector<int> targetPages;
      if (rangeSpec.isEmpty()) {
        for (int p = 1; p <= doc.pageCount(); ++p) {
          targetPages.append(p);
        }
      } else {
        QVector<QPair<int, int>> ranges;
        QString rangeErr;
        if (!parsePageRanges(rangeSpec, doc.pageCount(), ranges, rangeErr)) {
          failedDocs++;
          errors.append(QString("%1: %2").arg(sourceInfo.fileName(), rangeErr));
          QMetaObject::invokeMethod(
              this,
              [this, sourcePath, rangeErr]() {
                m_dragWidget->setFileStatus(
                    sourcePath, DropFileWidget::FileStatus::Failed, rangeErr);
              },
              Qt::QueuedConnection);
          continue;
        }
        for (const auto &r : ranges) {
          for (int p = r.first; p <= r.second; ++p) {
            if (!targetPages.contains(p)) {
              targetPages.append(p);
            }
          }
        }
      }

      int exportedForThisPdf = 0;
      QString docError;
      for (int pageNum : targetPages) {
        if (m_isCancelled) {
          break;
        }
        const int pageIdx = pageNum - 1;
        const QSizeF ptSize = doc.pagePointSize(pageIdx);
        const QSize pixelSize(qMax(1, qRound(ptSize.width() * dpi / 72.0)),
                              qMax(1, qRound(ptSize.height() * dpi / 72.0)));

        const QImage::Format imgFormat =
            (format == "jpg") ? QImage::Format_RGB32 : QImage::Format_ARGB32;
        const QImage pageImg =
            renderPdfPageOnWhite(doc, pageIdx, pixelSize, imgFormat);
        if (pageImg.isNull()) {
          docError = QString("Failed to render page %1.").arg(pageNum);
          break;
        }

        const QString baseStem = QString("%1_page_%2")
                                     .arg(sourceInfo.baseName())
                                     .arg(pageNum, 3, 10, QLatin1Char('0'));
        QString uniqueStem = baseStem;
        int suffix = 1;
        while (usedFileNames.contains(uniqueStem.toLower()) ||
               QFile::exists(
                   QDir(outputDir).filePath(uniqueStem + "." + format))) {
          uniqueStem = QString("%1_%2").arg(baseStem).arg(suffix++);
        }
        usedFileNames.insert(uniqueStem.toLower());
        const QString outImagePath =
            QDir(outputDir).filePath(uniqueStem + "." + format);

        QSaveFile saveFile(outImagePath);
        saveFile.setDirectWriteFallback(false);
        if (!saveFile.open(QIODevice::WriteOnly) ||
            !pageImg.save(&saveFile, format.toUpper().toUtf8().constData(),
                          effectiveQuality) ||
            !saveFile.commit()) {
          docError = QString("Failed to save image '%1'.")
                         .arg(QFileInfo(outImagePath).fileName());
          break;
        }
        exportedForThisPdf++;
        totalExportedImages++;
      }

      doc.close();

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, exportedForThisPdf, docError, format, outputDir,
           i]() {
            using FileStatus = DropFileWidget::FileStatus;
            if (m_isCancelled) {
              m_dragWidget->setFileStatus(sourcePath, FileStatus::Cancelled,
                                          "Export cancelled");
            } else if (docError.isEmpty() && exportedForThisPdf > 0) {
              m_dragWidget->setFileStatus(
                  sourcePath, FileStatus::Succeeded,
                  QString("Exported %1 %2 image(s) to %3")
                      .arg(exportedForThisPdf)
                      .arg(format.toUpper())
                      .arg(outputDir));
            } else {
              m_dragWidget->setFileStatus(sourcePath, FileStatus::Failed,
                                          docError);
            }
            if (!m_isCancelled) {
              m_progressBar->setValue(i + 1);
            }
          },
          Qt::QueuedConnection);

      if (!docError.isEmpty() && !m_isCancelled) {
        failedDocs++;
        errors.append(QString("%1: %2").arg(sourceInfo.fileName(), docError));
      }
    }

    QMetaObject::invokeMethod(
        this,
        [this, totalExportedImages, failedDocs, format, dpi, outputDir,
         errors]() {
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (m_isCancelled) {
            ui::Toast::show(this, "Cancelled",
                            "Page image export was cancelled.",
                            ui::ToastVariant::Default);
          } else if (totalExportedImages > 0 && failedDocs == 0) {
            ui::Dialog::info(
                this, "PDF Pages Exported",
                QString("Exported %1 %2 image(s) at %3 DPI to:\n%4")
                    .arg(totalExportedImages)
                    .arg(format.toUpper())
                    .arg(dpi)
                    .arg(outputDir));
          } else if (totalExportedImages > 0) {
            ui::Dialog::info(
                this, "Export Completed with Warnings",
                QString("Exported %1 %2 image(s) to:\n%3\n\nErrors:\n%4")
                    .arg(totalExportedImages)
                    .arg(format.toUpper())
                    .arg(outputDir)
                    .arg(errors.join("\n")));
          } else {
            ui::Toast::show(this, "Export Failed",
                            errors.isEmpty()
                                ? "Could not export pages to images."
                                : errors.join("\n"),
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

// ============================================================================
// Tool 5: Images to Merged PDF (Multiple PNG/JPG/WEBP -> 1 Multi-Page PDF)
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

void PdfPage::imagesToPdf() {
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
  const QVector<qreal> marginPts = {0.0, 18.0, 36.0, 54.0};
  const qreal marginPt = marginPts.value(marginIndex, 0.0);

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setIndeterminate(false);
  m_progressBar->setRange(0, orderedImages.size());
  m_progressBar->setValue(0, false);
  m_progressLabel->setText(
      QString("Combining %1 image(s) into PDF...").arg(orderedImages.size()));

  for (const QString &imgPath : orderedImages) {
    m_imageDragWidget->setFileStatus(
        imgPath, DropFileWidget::FileStatus::Processing, "Adding image to PDF");
  }

  QFuture<void> future = QtConcurrent::run([this, orderedImages, outputPath,
                                            pageSizeIndex, orientIndex,
                                            marginPt]() {
    QString error;
    bool succeeded = false;
    qint64 outputBytes = 0;
    const QFileInfo destinationInfo(outputPath);

    QTemporaryDir tempDir(
        destinationInfo.absoluteDir().filePath(".img-to-pdf-XXXXXX"));
    if (!tempDir.isValid()) {
      error = tempDir.errorString();
    } else {
      const QString candidatePath = tempDir.filePath("candidate.pdf");
      QPdfWriter writer(candidatePath);
      writer.setResolution(300);
      std::unique_ptr<QPainter> painter;

      for (int i = 0; i < orderedImages.size(); ++i) {
        if (m_isCancelled) {
          break;
        }
        const QString &imgPath = orderedImages.at(i);
        QImage rawImg(imgPath);
        if (rawImg.isNull()) {
          error = QString("Cannot load image '%1'.")
                      .arg(QFileInfo(imgPath).fileName());
          break;
        }

        // Composite onto opaque white so alpha channels render cleanly on white
        // paper
        QImage img(rawImg.size(), QImage::Format_RGB32);
        img.fill(Qt::white);
        {
          QPainter imgPainter(&img);
          imgPainter.drawImage(0, 0, rawImg);
        }

        if (pageSizeIndex == 0) {
          // Fit to Image
          double dpiX = rawImg.dotsPerMeterX() > 0
                            ? rawImg.dotsPerMeterX() * 0.0254
                            : 96.0;
          double dpiY = rawImg.dotsPerMeterY() > 0
                            ? rawImg.dotsPerMeterY() * 0.0254
                            : 96.0;
          if (dpiX < 36.0 || dpiX > 1200.0)
            dpiX = 96.0;
          if (dpiY < 36.0 || dpiY > 1200.0)
            dpiY = 96.0;
          const qreal wPt =
              qMax(72.0, img.width() * 72.0 / dpiX) + 2.0 * marginPt;
          const qreal hPt =
              qMax(72.0, img.height() * 72.0 / dpiY) + 2.0 * marginPt;
          QPageLayout layout(QPageSize(QSizeF(wPt, hPt), QPageSize::Point),
                             QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                             QPageLayout::Point);
          writer.setPageLayout(layout);
        } else {
          const QPageSize pageSize(pageSizeIndex == 1 ? QPageSize::A4
                                                      : QPageSize::Letter);
          QPageLayout::Orientation orient = QPageLayout::Portrait;
          if (orientIndex == 0) {
            orient = (img.width() > img.height()) ? QPageLayout::Landscape
                                                  : QPageLayout::Portrait;
          } else if (orientIndex == 2) {
            orient = QPageLayout::Landscape;
          }
          QPageLayout layout(pageSize, orient, QMarginsF(0, 0, 0, 0),
                             QPageLayout::Point);
          writer.setPageLayout(layout);
        }

        if (!painter) {
          painter = std::make_unique<QPainter>(&writer);
          if (!painter->isActive()) {
            error = "Failed to initialize PDF writer.";
            break;
          }
        } else {
          writer.newPage();
        }

        const qreal devW = painter->device()->width();
        const qreal devH = painter->device()->height();
        painter->fillRect(QRectF(0, 0, devW, devH), Qt::white);

        const qreal marginPx = marginPt * 300.0 / 72.0;
        const QRectF availRect(marginPx, marginPx,
                               qMax(1.0, devW - 2.0 * marginPx),
                               qMax(1.0, devH - 2.0 * marginPx));
        const qreal scale = qMin(availRect.width() / img.width(),
                                 availRect.height() / img.height());
        const qreal drawW = img.width() * scale;
        const qreal drawH = img.height() * scale;
        const QRectF targetRect(
            availRect.left() + (availRect.width() - drawW) / 2.0,
            availRect.top() + (availRect.height() - drawH) / 2.0, drawW, drawH);

        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->drawImage(targetRect, img);

        QMetaObject::invokeMethod(
            this,
            [this, done = i + 1, total = orderedImages.size()]() {
              if (!m_isCancelled) {
                m_progressBar->setValue(done);
                m_progressLabel->setText(
                    QString("Added %1 of %2 image(s)...").arg(done).arg(total));
              }
            },
            Qt::QueuedConnection);
      }

      if (painter) {
        painter->end();
      }

      if (error.isEmpty() && !m_isCancelled) {
        QPdfDocument verifyDoc;
        if (verifyDoc.load(candidatePath) != QPdfDocument::Error::None ||
            verifyDoc.pageCount() != orderedImages.size()) {
          error = "Generated PDF failed validation.";
        } else {
          verifyDoc.close();
          if (savePdfAtomically(candidatePath,
                                destinationInfo.absoluteFilePath(),
                                m_isCancelled, error)) {
            succeeded = true;
            outputBytes = QFileInfo(destinationInfo.absoluteFilePath()).size();
          }
        }
      }
    }

    QMetaObject::invokeMethod(
        this,
        [this, orderedImages, outputPath, succeeded, outputBytes, error]() {
          using FileStatus = DropFileWidget::FileStatus;
          for (const QString &imgPath : orderedImages) {
            if (m_isCancelled) {
              m_imageDragWidget->setFileStatus(imgPath, FileStatus::Cancelled,
                                               "Creation cancelled");
            } else if (succeeded) {
              m_imageDragWidget->setFileStatus(
                  imgPath, FileStatus::Succeeded,
                  QString("Added to %1").arg(outputPath));
            } else {
              m_imageDragWidget->setFileStatus(imgPath, FileStatus::Failed,
                                               error);
            }
          }
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (m_isCancelled) {
            ui::Toast::show(this, "Cancelled",
                            "Image-to-PDF creation was cancelled.",
                            ui::ToastVariant::Default);
          } else if (succeeded) {
            ui::Dialog::info(
                this, "Merged PDF Created",
                QString("Combined %1 image(s) into:\n%2\n\nOutput Size: %3")
                    .arg(orderedImages.size())
                    .arg(outputPath)
                    .arg(DropFileWidget::formatFileSize(outputBytes)));
          } else {
            ui::Toast::show(this, "PDF Creation Failed", error,
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

// ============================================================================
// Tool 6: PDF to Grayscale (Color PDF -> Grayscale PDF)
// ============================================================================

bool PdfPage::convertPdfToGrayscaleWithQtPdf(QPdfDocument &sourceDoc,
                                             const QString &tempOutputPath,
                                             int quality, QString &error) {
  const int clampedQuality = qBound(0, quality, 100);
  const int dpi =
      (clampedQuality >= 80) ? 300 : (100 + (200 * clampedQuality) / 80);

  QPdfWriter writer(tempOutputPath);
  writer.setResolution(dpi);
  std::unique_ptr<QPainter> painter;

  for (int page = 0; page < sourceDoc.pageCount(); ++page) {
    if (m_isCancelled) {
      return false;
    }
    const QSizeF ptSize = sourceDoc.pagePointSize(page);
    QPageLayout layout(QPageSize(ptSize, QPageSize::Point),
                       QPageLayout::Portrait, QMarginsF(0, 0, 0, 0),
                       QPageLayout::Point);
    writer.setPageLayout(layout);
    if (!painter) {
      painter = std::make_unique<QPainter>(&writer);
      if (!painter->isActive()) {
        error = "Failed to initialize PDF writer.";
        return false;
      }
    } else {
      writer.newPage();
    }

    const QSize renderSize(qMax(1, qRound(ptSize.width() * dpi / 72.0)),
                           qMax(1, qRound(ptSize.height() * dpi / 72.0)));
    const QImage colorImg = renderPdfPageOnWhite(sourceDoc, page, renderSize);
    if (colorImg.isNull()) {
      error = QString("Failed to render page %1.").arg(page + 1);
      return false;
    }
    const QImage grayImg = colorImg.convertToFormat(QImage::Format_Grayscale8);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(
        QRectF(0, 0, painter->device()->width(), painter->device()->height()),
        grayImg);
  }

  if (painter) {
    painter->end();
  }
  return QFileInfo(tempOutputPath).size() > 0;
}

PdfPage::CompressionResult
PdfPage::convertPdfToGrayscale(const QString &inputPath,
                               const QString &outputPath, int quality) {
  CompressionResult result;
  if (m_isCancelled) {
    result.status = CompressionStatus::Cancelled;
    return result;
  }

  const QFileInfo sourceInfo(inputPath);
  const QFileInfo destinationInfo(outputPath);
  result.originalBytes = sourceInfo.size();
  const QDateTime sourceModified = sourceInfo.lastModified();
  if (!sourceInfo.isFile() || result.originalBytes <= 0) {
    result.error = "The input PDF is missing or empty.";
    return result;
  }
  if (isSamePdf(sourceInfo, destinationInfo)) {
    result.error = "Choose a different output file. The original PDF must not "
                   "be overwritten.";
    return result;
  }

  QPdfDocument sourceDocument;
  const auto sourceError = sourceDocument.load(sourceInfo.absoluteFilePath());
  if (sourceError != QPdfDocument::Error::None ||
      sourceDocument.pageCount() <= 0) {
    result.error = QString("Cannot open the input PDF (PDF error %1). "
                           "Check whether it is damaged or password-protected.")
                       .arg(static_cast<int>(sourceError));
    return result;
  }

  QTemporaryDir temporaryDirectory(
      destinationInfo.absoluteDir().filePath(".pdf-grayscale-XXXXXX"));
  if (!temporaryDirectory.isValid()) {
    result.error = QString("Cannot create temporary output in the destination "
                           "directory: %1")
                       .arg(temporaryDirectory.errorString());
    return result;
  }
  const QString candidatePath = temporaryDirectory.filePath("candidate.pdf");
  const QString executable = findGhostscriptExecutable();

  bool converted = false;
  if (!executable.isEmpty()) {
    converted =
        compressPdfWithGhostscript(executable, sourceInfo.absoluteFilePath(),
                                   candidatePath, quality, result.error, true);
  } else {
    converted = convertPdfToGrayscaleWithQtPdf(sourceDocument, candidatePath,
                                               quality, result.error);
  }

  if (!converted) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
    }
    return result;
  }

  QPdfDocument candidateDocument;
  const auto candidateError = candidateDocument.load(candidatePath);
  if (candidateError != QPdfDocument::Error::None ||
      candidateDocument.pageCount() != sourceDocument.pageCount()) {
    result.error = "The grayscale PDF failed validation (unreadable output "
                   "or changed page count). No destination was replaced.";
    return result;
  }
  for (int page = 0; page < sourceDocument.pageCount(); ++page) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
      return result;
    }
    const QSizeF before = sourceDocument.pagePointSize(page);
    const QSizeF after = candidateDocument.pagePointSize(page);
    if (qAbs(before.width() - after.width()) > 0.5 ||
        qAbs(before.height() - after.height()) > 0.5) {
      result.error = "The grayscale PDF changed page dimensions. "
                     "No destination was replaced.";
      return result;
    }
  }
  candidateDocument.close();
  sourceDocument.close();

  if (m_isCancelled) {
    result.status = CompressionStatus::Cancelled;
    return result;
  }
  const QFileInfo currentSource(sourceInfo.absoluteFilePath());
  if (currentSource.size() != result.originalBytes ||
      currentSource.lastModified() != sourceModified) {
    result.error = "The input PDF changed during conversion. Please retry.";
    return result;
  }
  const qint64 candidateBytes = QFileInfo(candidatePath).size();
  if (candidateBytes <= 0) {
    result.error = "The grayscale PDF is empty. No destination was replaced.";
    return result;
  }
  if (isSamePdf(currentSource, QFileInfo(destinationInfo.absoluteFilePath()))) {
    result.error = "The destination now points to the input PDF. "
                   "No destination was replaced.";
    return result;
  }
  if (!savePdfAtomically(candidatePath, destinationInfo.absoluteFilePath(),
                         m_isCancelled, result.error)) {
    if (m_isCancelled) {
      result.status = CompressionStatus::Cancelled;
    }
    return result;
  }
  result.status = CompressionStatus::Compressed;
  result.outputBytes = candidateBytes;
  return result;
}

void PdfPage::convertSinglePdfToGrayscale(const QString &sourcePath,
                                          int quality) {
  QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Grayscale PDF",
      QFileInfo(sourcePath)
          .dir()
          .filePath(QFileInfo(sourcePath).baseName() + "_grayscale.pdf"),
      "PDF Files (*.pdf)");

  if (outputPath.isEmpty()) {
    return;
  }

  m_isCancelled = false;
  setProcessingState(true);
  m_progressBar->setRange(0, 1);
  m_progressBar->setValue(0, false);
  m_progressBar->setIndeterminate(true);
  m_progressLabel->setText(QString("Converting %1 to grayscale (1 of 1)...")
                               .arg(QFileInfo(sourcePath).fileName()));
  m_dragWidget->setFileStatus(sourcePath,
                              DropFileWidget::FileStatus::Processing,
                              "Converting PDF to grayscale");

  QFuture<void> future = QtConcurrent::run([this, sourcePath, outputPath,
                                            quality]() {
    const CompressionResult result =
        convertPdfToGrayscale(sourcePath, outputPath, quality);

    QMetaObject::invokeMethod(
        this,
        [this, sourcePath, result, outputPath]() {
          using FileStatus = DropFileWidget::FileStatus;
          FileStatus status = FileStatus::Failed;
          QString detail = result.error;
          switch (result.status) {
          case CompressionStatus::Compressed:
            status = FileStatus::Succeeded;
            detail = QString("Saved grayscale PDF to %1").arg(outputPath);
            break;
          case CompressionStatus::Cancelled:
            status = FileStatus::Cancelled;
            detail = "Conversion cancelled; no output saved";
            break;
          case CompressionStatus::Unchanged:
          case CompressionStatus::Failed:
            break;
          }
          m_dragWidget->setFileStatus(sourcePath, status, detail);
          if (result.status != CompressionStatus::Cancelled) {
            m_progressBar->setValue(1, false);
          }
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          if (result.status == CompressionStatus::Compressed) {
            ui::Dialog::info(
                this, "Grayscale PDF Saved",
                QString("Saved to: %1\n\nOriginal Size: %2\nGrayscale Size: %3")
                    .arg(outputPath)
                    .arg(DropFileWidget::formatFileSize(result.originalBytes))
                    .arg(DropFileWidget::formatFileSize(result.outputBytes)));
          } else if (result.status == CompressionStatus::Cancelled) {
            ui::Toast::show(this, "Cancelled",
                            "Grayscale conversion was cancelled.",
                            ui::ToastVariant::Default);
          } else {
            ui::Toast::show(this, "Grayscale Conversion Failed", result.error,
                            ui::ToastVariant::Destructive);
          }
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}

void PdfPage::convertBatchPdfToGrayscale(const QStringList &sourcePaths,
                                         int quality) {
  QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Grayscale PDFs", QDir::homePath());

  if (outputDir.isEmpty()) {
    return;
  }

  m_isCancelled = false;
  setProcessingState(true);
  const int totalFiles = sourcePaths.size();
  m_progressBar->setIndeterminate(false);
  m_progressBar->setRange(0, totalFiles);
  m_progressBar->setValue(0, false);
  m_progressLabel->setText(
      QString("Converting 0 of %1 PDFs to grayscale...").arg(totalFiles));

  QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                            quality, totalFiles]() {
    int successCount = 0;
    int failureCount = 0;
    qint64 totalOrigBytes = 0;
    qint64 totalOutputBytes = 0;
    QStringList errors;
    QSet<QString> usedBaseNames;

    for (int i = 0; i < totalFiles; ++i) {
      if (m_isCancelled) {
        break;
      }
      const QString &sourcePath = sourcePaths.at(i);
      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, i, totalFiles]() {
            m_dragWidget->setFileStatus(sourcePath,
                                        DropFileWidget::FileStatus::Processing,
                                        "Converting PDF to grayscale");
            m_progressLabel->setText(QString("Converting %1 (%2 of %3)...")
                                         .arg(QFileInfo(sourcePath).fileName())
                                         .arg(i + 1)
                                         .arg(totalFiles));
          },
          Qt::QueuedConnection);

      QFileInfo fileInfo(sourcePath);
      QString baseName = fileInfo.baseName() + "_grayscale";
      QString uniqueBaseName = baseName;
      int suffix = 1;
      while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
             QFile::exists(QDir(outputDir).filePath(uniqueBaseName + ".pdf"))) {
        uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
      }
      usedBaseNames.insert(uniqueBaseName.toLower());
      const QString outputPath =
          QDir(outputDir).filePath(uniqueBaseName + ".pdf");

      const CompressionResult result =
          convertPdfToGrayscale(sourcePath, outputPath, quality);

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, outputPath, result, i, totalFiles]() {
            using FileStatus = DropFileWidget::FileStatus;
            FileStatus status = FileStatus::Failed;
            QString detail = result.error;
            if (result.status == CompressionStatus::Compressed) {
              status = FileStatus::Succeeded;
              detail = QString("Saved to %1").arg(outputPath);
            } else if (result.status == CompressionStatus::Cancelled) {
              status = FileStatus::Cancelled;
              detail = "Conversion cancelled; no output saved";
            }
            m_dragWidget->setFileStatus(sourcePath, status, detail);
            if (result.status != CompressionStatus::Cancelled) {
              m_progressBar->setValue(i + 1);
              m_progressLabel->setText(QString("Processed %1 of %2 PDFs")
                                           .arg(i + 1)
                                           .arg(totalFiles));
            }
          },
          Qt::QueuedConnection);

      if (result.status == CompressionStatus::Cancelled) {
        break;
      }
      if (result.status == CompressionStatus::Compressed) {
        successCount++;
        totalOrigBytes += result.originalBytes;
        totalOutputBytes += result.outputBytes;
      } else {
        failureCount++;
        if (errors.size() < 3) {
          errors.append(
              QString("%1: %2").arg(fileInfo.fileName(), result.error));
        }
      }
    }

    QMetaObject::invokeMethod(
        this,
        [this, successCount, failureCount, totalFiles, totalOrigBytes,
         totalOutputBytes, errors]() {
          setProcessingState(false);
          m_cancelButton->setText(cancelButtonTextForMode(m_currentTool));

          const QString title = m_isCancelled
                                    ? "Batch Grayscale Conversion Cancelled"
                                    : "Batch Grayscale Conversion Complete";
          QString message = QString("%1 PDF(s) converted to grayscale.\n"
                                    "%2 PDF(s) failed.")
                                .arg(successCount)
                                .arg(failureCount);

          if (totalOrigBytes > 0 && totalOutputBytes > 0 && successCount > 0) {
            message +=
                QString("\n\nTotal Original: %1\nTotal Grayscale: %2")
                    .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                    .arg(DropFileWidget::formatFileSize(totalOutputBytes));
          }
          if (!errors.isEmpty()) {
            message += "\n\n" + errors.join("\n");
          }
          if (m_isCancelled) {
            message += QString("\n%1 PDF(s) not completed due to cancellation.")
                           .arg(totalFiles - successCount - failureCount);
          }
          ui::Dialog::info(this, title, message);
        },
        Qt::QueuedConnection);
  });
  m_futureWatcher->setFuture(future);
}
