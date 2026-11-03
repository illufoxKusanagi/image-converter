#ifndef PDF_TOOL_CONTROLLER_H
#define PDF_TOOL_CONTROLLER_H

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <atomic>

class PdfToolController : public QObject {
  Q_OBJECT

public:
  enum class FileStatus {
    Waiting = 0,
    Processing,
    Succeeded,
    Unchanged,
    Failed,
    Cancelled
  };
  Q_ENUM(FileStatus)

  explicit PdfToolController(QObject *parent = nullptr);
  ~PdfToolController() override;

  virtual void cancel();
  void cancelAndWait();
  bool isRunning() const;
  bool isCancelled() const;

signals:
  void progressChanged(int current, int total, const QString &statusText);
  void fileStatusChanged(const QString &filePath, int status, const QString &detail);

protected:
  void waitForRunningJob();

  std::atomic<bool> m_isCancelled{false};
  QFutureWatcher<void> m_watcher;
};

#endif // PDF_TOOL_CONTROLLER_H

