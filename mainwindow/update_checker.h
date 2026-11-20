#ifndef UPDATE_CHECKER_H
#define UPDATE_CHECKER_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QWidget>

class QNetworkAccessManager;
class QNetworkReply;

class UpdateChecker final : public QObject {
  Q_OBJECT

public:
  explicit UpdateChecker(QWidget *dialogParent, QObject *parent = nullptr);
  ~UpdateChecker() override = default;

  static QString currentVersion();

  void checkIfWeeklyIntervalElapsed();
  void checkManually();
  bool isChecking() const;

signals:
  void checkingChanged(bool checking);

private:
  void startCheck(bool manual);
  void handleReply(QNetworkReply *reply, bool manual);
  void showUpdateDialog(const QString &latestVersion);

  QPointer<QWidget> m_dialogParent;
  QNetworkAccessManager *m_networkManager{nullptr};
  bool m_isChecking{false};
};

#endif // UPDATE_CHECKER_H
