#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "widgets/button_action.h"
#include "widgets/drop_file_widget.h"
#include "widgets/input_widget.h"
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
  void onPresetChanged();
  void onQualitySliderChanged();

private:
  QVBoxLayout *mainLayout;
  DropFileWidget *m_dragWidget;
  SliderWidget *m_qualitySlider;
  InputWidget *m_presetDropdown;
  QProgressBar *m_progressBar;
  ButtonAction *m_processButton;
  ButtonAction *m_cancelButton;
  QFutureWatcher<void> *m_futureWatcher;
  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};

  void setupImageLayout();
  void setupExtensionButton();
  void setupImageAttribute();
  void setProcessingState(bool isProcessing);

  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  bool compressPdf(const QString &inputPath, const QString &outputPath,
                   int quality,
                   const std::function<void(int, int)> &progressCallback);
  QImage compressImage(const QImage &originalImage, int quality);
  int calculateDPI(int quality);
  qreal calculateScaleFactor(int quality);
};

#endif // PDF_PAGE_H
