#include "split_pdf_panel.h"
#include "controllers/pdf/pdf_utils.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPair>
#include <QPdfDocument>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

SplitPdfPanel::SplitPdfPanel(QWidget *parent) : QWidget(parent) {
  auto *splitLayout = new QHBoxLayout(this);
  splitLayout->setContentsMargins(0, 0, 0, 0);
  splitLayout->setSpacing(12);

  const QStringList splitModes = {"Custom Page Ranges (e.g. 1-3, 5, 8-12)",
                                  "Fixed Interval (Every N Pages)",
                                  "Extract All Pages (1 PDF per page)"};
  m_modeSelect = new ui::Select(splitModes, this);
  m_modeField = new ui::FormField("Split Mode", m_modeSelect, this);
  connect(m_modeSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &SplitPdfPanel::onModeChanged);

  m_rangesInput = new ui::Input("e.g. 1-3, 5, 8-12", this);
  m_rangesInput->setText("1");
  m_rangesField =
      new ui::FormField("Page Ranges (1-based)", m_rangesInput, this);

  m_intervalInput = new ui::NumberInput(this);
  m_intervalInput->setRange(1, 1000);
  m_intervalInput->setValue(1);
  m_intervalInput->setSuffix(" page(s)");
  m_intervalField =
      new ui::FormField("Interval Size", m_intervalInput, this);

  splitLayout->addWidget(m_modeField, 2, Qt::AlignTop);
  splitLayout->addWidget(m_rangesField, 2, Qt::AlignTop);
  splitLayout->addWidget(m_intervalField, 1, Qt::AlignTop);

  m_controller = new SplitController(this);
  connect(m_controller, &SplitController::finished, this,
          &SplitPdfPanel::onSplitFinished, Qt::QueuedConnection);

  onModeChanged(0);
}

int SplitPdfPanel::splitMode() const {
  return m_modeSelect ? m_modeSelect->currentIndex() : 0;
}

QString SplitPdfPanel::rangeSpec() const {
  return m_rangesInput ? m_rangesInput->text().trimmed() : QString();
}

int SplitPdfPanel::interval() const {
  return m_intervalInput ? qMax(1, m_intervalInput->value()) : 1;
}

bool SplitPdfPanel::execute(const QStringList &sourcePaths) {
  QWidget *const toastParent = window() ? window() : this;
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(toastParent, "No PDF Selected",
                    "Please select at least one PDF file to split.",
                    ui::ToastVariant::Warning);
    return false;
  }

  if (!m_controller) {
    return false;
  }

  const int activeMode = splitMode();
  const QString ranges = rangeSpec();
  const int intervalPages = interval();

  // Pre-validate the first PDF and range syntax before prompting for folder
  if (activeMode == 0) {
    QPdfDocument checkDoc;
    if (checkDoc.load(sourcePaths.first()) != QPdfDocument::Error::None ||
        checkDoc.pageCount() <= 0) {
      ui::Toast::show(toastParent, "Invalid PDF",
                      QString("Cannot open '%1'.")
                          .arg(QFileInfo(sourcePaths.first()).fileName()),
                      ui::ToastVariant::Destructive);
      return false;
    }
    QVector<QPair<int, int>> testRanges;
    QString rangeErr;
    if (!pdfutil::parsePageRanges(ranges, checkDoc.pageCount(), testRanges,
                                  rangeErr)) {
      ui::Toast::show(toastParent, "Invalid Page Range", rangeErr,
                      ui::ToastVariant::Warning);
      return false;
    }
  }

  const QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Split PDFs", QDir::homePath());
  if (outputDir.isEmpty()) {
    return false;
  }

  SplitController::Params params;
  params.sourcePaths = sourcePaths;
  params.outputDir = outputDir;
  params.mode = activeMode;
  params.rangeSpec = ranges;
  params.interval = intervalPages;

  m_controller->start(params);
  return true;
}

void SplitPdfPanel::onModeChanged(int index) {
  if (m_rangesInput) {
    m_rangesInput->setEnabled(index == 0);
  }
  if (m_intervalInput) {
    m_intervalInput->setEnabled(index == 1);
  }
}

void SplitPdfPanel::onSplitFinished(const SplitController::Result &result) {
  emit executionFinished();

  QWidget *const toastParent = window() ? window() : this;
  if (result.cancelled) {
    ui::Toast::show(toastParent, "Cancelled", "PDF split was cancelled.",
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
    ui::Toast::show(toastParent, "PDF Split Failed",
                    result.errors.isEmpty() ? "Could not split PDF."
                                            : result.errors.join("\n"),
                    ui::ToastVariant::Destructive);
  }
}
