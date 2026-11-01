#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>

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
  ui::Progress *m_progressBar{nullptr};
  QLabel *m_progressLabel{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};
  QFutureWatcher<void> *m_futureWatcher{nullptr};
  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};

  void setProcessingState(bool isProcessing);

  enum class CompressionStatus { Compressed, Unchanged, Failed, Cancelled };
  struct CompressionResult {
    CompressionStatus status{CompressionStatus::Failed};
    qint64 originalBytes{0};
    qint64 outputBytes{0};
    QString error;
  };

  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  CompressionResult compressPdf(const QString &inputPath,
                                const QString &outputPath, int quality);
  bool compressPdfWithGhostscript(const QString &executable,
                                  const QString &inputPath,
                                  const QString &tempOutputPath, int quality,
                                  QString &error);
  static QString findGhostscriptExecutable();
};

#endif // PDF_PAGE_H
