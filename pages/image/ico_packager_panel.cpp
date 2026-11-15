#include "ico_packager_panel.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QString>
#include <QVBoxLayout>

IcoPackagerPanel::IcoPackagerPanel(QWidget *parent) : QWidget(parent) {
  auto *icoLayout = new QVBoxLayout(this);
  icoLayout->setContentsMargins(0, 0, 0, 0);
  icoLayout->setSpacing(8);

  m_hintLabel = new QLabel(
      "Checked icon resolutions are rendered with smooth aspect-preserving "
      "scaling and packed into a single multi-resolution .ico file:",
      this);
  m_hintLabel->setWordWrap(true);

  auto *icoSizesRow = new QHBoxLayout();
  icoSizesRow->setContentsMargins(0, 0, 0, 0);
  icoSizesRow->setSpacing(16);

  const QVector<int> icoSizes = {16, 32, 48, 64, 128, 256};
  m_sizeToggles.reserve(icoSizes.size());
  for (int sz : icoSizes) {
    auto *sw = new ui::Switch(true, this);
    auto *lbl = new QLabel(QString("%1×%1 px").arg(sz), this);
    auto *pairLayout = new QHBoxLayout();
    pairLayout->setContentsMargins(0, 0, 0, 0);
    pairLayout->setSpacing(6);
    pairLayout->addWidget(sw);
    pairLayout->addWidget(lbl);
    icoSizesRow->addLayout(pairLayout);
    m_sizeToggles.push_back({sz, sw, lbl});
  }
  icoSizesRow->addStretch(1);

  icoLayout->addWidget(m_hintLabel);
  icoLayout->addLayout(icoSizesRow);

  m_controller = new IcoExportController(this);
  connect(m_controller, &IcoExportController::finished, this,
          &IcoPackagerPanel::onIcoExportFinished, Qt::QueuedConnection);

  applyThemeStyles();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &IcoPackagerPanel::applyThemeStyles);
}

void IcoPackagerPanel::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  if (m_hintLabel) {
    m_hintLabel->setFont(t.font(t.sizeXs, QFont::Medium));
    m_hintLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  const QString labelStyle =
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground));
  for (const IcoSizeToggle &item : m_sizeToggles) {
    if (item.label) {
      item.label->setFont(t.font(t.sizeSm, QFont::DemiBold));
      item.label->setStyleSheet(labelStyle);
    }
  }
}

QVector<int> IcoPackagerPanel::selectedSizes() const {
  QVector<int> sizes;
  sizes.reserve(m_sizeToggles.size());
  for (const IcoSizeToggle &item : m_sizeToggles) {
    if (item.toggle && item.toggle->isChecked()) {
      sizes.append(item.size);
    }
  }
  return sizes;
}

bool IcoPackagerPanel::execute(const QStringList &sourcePaths) {
  QWidget *const toastParent = window() ? window() : this;

  if (sourcePaths.isEmpty()) {
    ui::Toast::show(toastParent, "No Image Selected",
                    "Please select at least one image or SVG file.",
                    ui::ToastVariant::Warning);
    return false;
  }

  const QVector<int> sizes = selectedSizes();
  if (sizes.isEmpty()) {
    ui::Toast::show(
        toastParent, "No Icon Sizes Selected",
        "Please enable at least one icon resolution (e.g. 32×32 or 256×256).",
        ui::ToastVariant::Warning);
    return false;
  }

  const QString sourceDir = QFileInfo(sourcePaths.first()).absolutePath();
  const QString defaultDir = sourceDir.isEmpty() ? QDir::homePath() : sourceDir;
  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for .ico Files", defaultDir);
  if (outputDir.isEmpty()) {
    return false;
  }

  if (!m_controller) {
    return false;
  }

  IcoExportController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.sizes = sizes;

  m_controller->start(params);
  return true;
}

void IcoPackagerPanel::onIcoExportFinished(
    const IcoExportController::Result &result) {
  emit executionFinished();

  QWidget *const toastParent = window() ? window() : this;

  if (result.cancelled) {
    ui::Toast::show(toastParent, "Cancelled", "Icon export was cancelled.",
                    ui::ToastVariant::Default);
    return;
  }

  if (result.iconsCreated > 0) {
    QStringList sizeLabels;
    sizeLabels.reserve(result.sizes.size());
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
    ui::Toast::show(toastParent, "Icon Export Failed",
                    result.error.isEmpty() ? "Failed to generate .ico files."
                                           : result.error,
                    ui::ToastVariant::Destructive);
  }
}
