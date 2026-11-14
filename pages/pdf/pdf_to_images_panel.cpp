#include "pdf_to_images_panel.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

PdfToImagesPanel::PdfToImagesPanel(QWidget *parent) : QWidget(parent) {
  auto *exportLayout = new QHBoxLayout(this);
  exportLayout->setContentsMargins(0, 0, 0, 0);
  exportLayout->setSpacing(12);

  const QStringList exportFormats = {"PNG", "JPG", "WEBP", "TIFF"};
  m_formatSelect = new ui::Select(exportFormats, this);
  m_formatField = new ui::FormField("Image Format", m_formatSelect, this);
  connect(m_formatSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfToImagesPanel::exportFormatChanged);

  const QStringList dpiOptions = {"72 DPI (Screen)", "150 DPI (Medium)",
                                  "300 DPI (Print)", "600 DPI (High Detail)"};
  m_dpiSelect = new ui::Select(dpiOptions, this);
  m_dpiSelect->setCurrentIndex(2); // Default 300 DPI
  m_dpiField = new ui::FormField("Render Resolution", m_dpiSelect, this);

  m_pageRangeInput = new ui::Input("All pages, or e.g. 1-3, 5", this);
  m_pageRangeField =
      new ui::FormField("Page Filter (optional)", m_pageRangeInput, this);

  exportLayout->addWidget(m_formatField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_dpiField, 1, Qt::AlignTop);
  exportLayout->addWidget(m_pageRangeField, 2, Qt::AlignTop);
}

QString PdfToImagesPanel::selectedFormat() const {
  const QStringList formats = {"png", "jpg", "webp", "tiff"};
  return formats.value(formatIndex(), "png");
}

int PdfToImagesPanel::formatIndex() const {
  return m_formatSelect ? m_formatSelect->currentIndex() : 0;
}

int PdfToImagesPanel::selectedDpi() const {
  const int dpiIndex = m_dpiSelect ? m_dpiSelect->currentIndex() : 2;
  const QVector<int> dpis = {72, 150, 300, 600};
  return dpis.value(dpiIndex, 300);
}

QString PdfToImagesPanel::pageRangeSpec() const {
  return m_pageRangeInput ? m_pageRangeInput->text().trimmed() : QString();
}
