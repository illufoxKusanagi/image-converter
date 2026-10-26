#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/message_box_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>
#include <functional>

class PdfPage : public QWidget {
  Q_OBJECT

public:
  explicit PdfPage(QWidget *parent = nullptr);
  ~PdfPage() override;

private slots:
  void onProcessButtonClicked();
  void onCancelButtonClicked();
  void onPresetChanged(int index);
  void onQualitySliderChanged();
  void applyThemeStyles();

private:
  QVBoxLayout *mainLayout{nullptr};
  DropFileWidget *m_dragWidget{nullptr};
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_presetSelect{nullptr};
  ui::FormField *m_presetField{nullptr};
  QProgressBar *m_progressBar{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};
  QFutureWatcher<void> *m_futureWatcher{nullptr};
  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};

  void setProcessingState(bool isProcessing);

  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  bool compressPdf(const QString &inputPath, const QString &outputPath,
                   int quality,
                   const std::function<void(int, int)> &onPageProgress);
  bool compressPdfWithGhostscript(const QString &inputPath,
                                  const QString &tempOutputPath, int quality);
  bool compressPdfWithQtFallback(
      const QString &inputPath, const QString &tempOutputPath, int quality,
      const std::function<void(int, int)> &onPageProgress);
  static QString findGhostscriptExecutable();
  QImage compressImage(const QImage &originalImage, int quality);
  int calculateDPI(int quality);
};

#endif // PDF_PAGE_H
