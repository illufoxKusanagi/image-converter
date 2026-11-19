#include "svg_options_panel.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/i18n.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtGlobal>

SvgOptionsPanel::SvgOptionsPanel(QWidget *parent) : QWidget(parent) {
  auto *svgRootLayout = new QVBoxLayout(this);
  svgRootLayout->setContentsMargins(0, 0, 0, 0);
  svgRootLayout->setSpacing(10);

  auto *svgTopRow = new QHBoxLayout();
  svgTopRow->setContentsMargins(0, 0, 0, 0);
  svgTopRow->setSpacing(12);

  const QStringList svgFormats = {"PNG", "WEBP", "JPG", "PDF"};
  m_formatSelect = new ui::Select(svgFormats, this);
  m_formatField = new ui::FormField(tr("Output Format"), m_formatSelect, this);
  connect(m_formatSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SvgOptionsPanel::onFormatChanged);

  const QStringList svgScales = {
      tr("1x (Original Size)"), tr("2x (Retina / HiDPI)"),
      tr("4x (Ultra HD)"), tr("Custom Dimensions (W × H)")};
  m_scaleSelect = new ui::Select(svgScales, this);
  m_scaleSelect->setCurrentIndex(1); // Default 2x crisp export
  m_scaleField =
      new ui::FormField(tr("Vector Scale Preset"), m_scaleSelect, this);
  connect(m_scaleSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SvgOptionsPanel::onScaleModeChanged);

  m_customWidthInput = new ui::Input("512", this);
  m_customWidthInput->setText("512");
  m_customWidthInput->setSuffixText("px");
  if (m_customWidthInput->lineEdit()) {
    m_customWidthInput->lineEdit()->setValidator(
        new QIntValidator(1, 16384, m_customWidthInput));
  }
  m_customWidthField =
      new ui::FormField(tr("Custom Width"), m_customWidthInput, this);

  m_customHeightInput = new ui::Input("512", this);
  m_customHeightInput->setText("512");
  m_customHeightInput->setSuffixText("px");
  if (m_customHeightInput->lineEdit()) {
    m_customHeightInput->lineEdit()->setValidator(
        new QIntValidator(1, 16384, m_customHeightInput));
  }
  m_customHeightField =
      new ui::FormField(tr("Custom Height"), m_customHeightInput, this);

  svgTopRow->addWidget(m_formatField, 1, Qt::AlignTop);
  svgTopRow->addWidget(m_scaleField, 2, Qt::AlignTop);
  svgTopRow->addWidget(m_customWidthField, 1, Qt::AlignTop);
  svgTopRow->addWidget(m_customHeightField, 1, Qt::AlignTop);

  auto *svgBottomRow = new QHBoxLayout();
  svgBottomRow->setContentsMargins(0, 0, 0, 0);
  svgBottomRow->setSpacing(16);

  m_qualitySlider = new SliderWidget(this, tr("Raster Quality"));
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setValue(90);

  m_keepAspectSwitch = new ui::Switch(true, this);
  m_keepAspectLabel = new QLabel(tr("Preserve Aspect Ratio"), this);

  m_whiteBgSwitch = new ui::Switch(false, this);
  m_whiteBgLabel = new QLabel(tr("White Background"), this);

  auto *aspectToggleRow = new QHBoxLayout();
  aspectToggleRow->setContentsMargins(0, 0, 0, 0);
  aspectToggleRow->setSpacing(8);
  aspectToggleRow->addWidget(m_keepAspectSwitch);
  aspectToggleRow->addWidget(m_keepAspectLabel);

  auto *bgToggleRow = new QHBoxLayout();
  bgToggleRow->setContentsMargins(0, 0, 0, 0);
  bgToggleRow->setSpacing(8);
  bgToggleRow->addWidget(m_whiteBgSwitch);
  bgToggleRow->addWidget(m_whiteBgLabel);

  svgBottomRow->addWidget(m_qualitySlider, 1, Qt::AlignVCenter);
  svgBottomRow->addLayout(aspectToggleRow);
  svgBottomRow->addLayout(bgToggleRow);

  svgRootLayout->addLayout(svgTopRow);
  svgRootLayout->addLayout(svgBottomRow);

  m_controller = new SvgConvertController(this);
  connect(m_controller, &SvgConvertController::finished, this,
          &SvgOptionsPanel::onSvgConvertFinished, Qt::QueuedConnection);

  onFormatChanged(0);
  onScaleModeChanged(m_scaleSelect->currentIndex());

  applyThemeStyles();
  retranslateUi();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &SvgOptionsPanel::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &SvgOptionsPanel::retranslateUi);
}

void SvgOptionsPanel::syncProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  if (m_formatSelect) {
    m_formatSelect->setEnabled(!isProcessing);
  }
  if (m_scaleSelect) {
    m_scaleSelect->setEnabled(!isProcessing);
  }
  onFormatChanged(m_formatSelect ? m_formatSelect->currentIndex() : 0);
  onScaleModeChanged(m_scaleSelect ? m_scaleSelect->currentIndex() : 0);
}

void SvgOptionsPanel::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  const QString labelStyle =
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground));
  for (QLabel *lbl : {m_keepAspectLabel, m_whiteBgLabel}) {
    if (lbl) {
      lbl->setFont(t.font(t.sizeSm, QFont::Medium));
      lbl->setStyleSheet(labelStyle);
    }
  }
}

bool SvgOptionsPanel::execute(const QStringList &sourcePaths) {
  QWidget *toastParent = window() ? window() : this;

  if (sourcePaths.isEmpty()) {
    ui::Toast::show(toastParent, tr("No SVG Files Selected"),
                    tr("Please select at least one .svg file."),
                    ui::ToastVariant::Warning);
    return false;
  }
  if (!m_controller) {
    return false;
  }

  const QString defaultDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, tr("Select Output Directory for Converted SVG Files"), defaultDir);
  if (outputDir.isEmpty()) {
    return false;
  }

  const QStringList fmtKeys = {"png", "webp", "jpg", "pdf"};
  const int fmtIdx = m_formatSelect ? m_formatSelect->currentIndex() : 0;

  SvgConvertController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.format = fmtKeys.value(fmtIdx, "png");
  params.scaleMode = m_scaleSelect ? m_scaleSelect->currentIndex() : 0;
  params.customWidth =
      m_customWidthInput ? m_customWidthInput->text().toInt() : 512;
  params.customHeight =
      m_customHeightInput ? m_customHeightInput->text().toInt() : 512;
  if (params.customWidth <= 0) {
    params.customWidth = 512;
  }
  if (params.customHeight <= 0) {
    params.customHeight = 512;
  }
  params.customWidth = qBound(1, params.customWidth, 16384);
  params.customHeight = qBound(1, params.customHeight, 16384);
  params.preserveAspectRatio =
      (params.scaleMode != 3) ||
      (m_keepAspectSwitch && m_keepAspectSwitch->isChecked());
  params.whiteBackground =
      (params.format == "jpg") ||
      (m_whiteBgSwitch && m_whiteBgSwitch->isChecked());
  params.quality = m_qualitySlider ? m_qualitySlider->getValue() : 90;

  m_controller->start(params);
  return true;
}

void SvgOptionsPanel::retranslateUi() {
  if (m_formatField) {
    m_formatField->setLabel(tr("Output Format"));
  }
  if (m_scaleField) {
    m_scaleField->setLabel(tr("Vector Scale Preset"));
  }
  if (m_scaleSelect) {
    const QSignalBlocker blocker(m_scaleSelect);
    const QStringList svgScales = {
        tr("1x (Original Size)"), tr("2x (Retina / HiDPI)"),
        tr("4x (Ultra HD)"), tr("Custom Dimensions (W × H)")};
    for (int i = 0; i < svgScales.size() && i < m_scaleSelect->count(); ++i) {
      m_scaleSelect->setItemText(i, svgScales.at(i));
    }
  }
  if (m_customWidthField) {
    m_customWidthField->setLabel(tr("Custom Width"));
  }
  if (m_customHeightField) {
    m_customHeightField->setLabel(tr("Custom Height"));
  }
  if (m_keepAspectLabel) {
    m_keepAspectLabel->setText(tr("Preserve Aspect Ratio"));
  }
  if (m_whiteBgLabel) {
    m_whiteBgLabel->setText(tr("White Background"));
  }
  onFormatChanged(m_formatSelect ? m_formatSelect->currentIndex() : 0);
}

void SvgOptionsPanel::onFormatChanged(int index) {
  const bool isLossy = (index == 1 || index == 2); // WEBP or JPG
  const bool isJpg = (index == 2);
  if (m_qualitySlider) {
    m_qualitySlider->setEnabled(isLossy && !m_isProcessing);
    if (index == 0) {
      m_qualitySlider->setTitle(tr("PNG (Lossless - Quality N/A)"));
    } else if (index == 3) {
      m_qualitySlider->setTitle(tr("Vector PDF (Resolution Independent)"));
    } else {
      m_qualitySlider->setTitle(tr("Raster Quality"));
    }
  }
  if (m_whiteBgSwitch) {
    if (isJpg) {
      m_whiteBgSwitch->setChecked(true);
      m_whiteBgSwitch->setEnabled(false);
    } else {
      m_whiteBgSwitch->setEnabled(!m_isProcessing);
    }
  }
}

void SvgOptionsPanel::onScaleModeChanged(int index) {
  const bool isCustom = (index == 3);
  if (m_customWidthInput) {
    m_customWidthInput->setEnabled(isCustom && !m_isProcessing);
  }
  if (m_customHeightInput) {
    m_customHeightInput->setEnabled(isCustom && !m_isProcessing);
  }
  if (m_keepAspectSwitch) {
    m_keepAspectSwitch->setEnabled(isCustom && !m_isProcessing);
  }
}

void SvgOptionsPanel::onSvgConvertFinished(
    const SvgConvertController::Result &result) {
  emit executionFinished();

  QWidget *toastParent = window() ? window() : this;
  if (result.cancelled) {
    ui::Toast::show(toastParent, tr("Cancelled"),
                    tr("SVG conversion was cancelled."),
                    ui::ToastVariant::Default);
    return;
  }

  if (result.filesConverted > 0) {
    QString msg = tr("Converted %1 SVG file(s) to %2 in:\n%3")
                      .arg(result.filesConverted)
                      .arg(result.format.toUpper())
                      .arg(result.outputDir);
    if (result.failedFiles > 0) {
      msg += "\n\n" + tr("%1 file(s) failed:\n%2")
                          .arg(result.failedFiles)
                          .arg(result.error);
    }
    ui::Dialog::info(this, tr("SVG Conversion Complete"), msg);
  } else {
    ui::Toast::show(toastParent, tr("SVG Conversion Failed"),
                    result.error.isEmpty() ? tr("Failed to convert SVG files.")
                                           : result.error,
                    ui::ToastVariant::Destructive);
  }
}
