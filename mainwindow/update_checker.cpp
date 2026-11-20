#include "mainwindow/update_checker.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>

namespace {
constexpr const char *kReleasesApiUrl =
    "https://api.github.com/repos/illufoxKusanagi/image-converter/releases/latest";
constexpr const char *kDownloadPageUrl =
    "https://ariefsatria.my.id/apps/image-converter";
constexpr const char *kSettingsLastCheckKey = "updates/lastCheckUtc";
constexpr qint64 kWeeklyIntervalSecs = 7LL * 24LL * 3600LL;
constexpr int kAutoCheckStartupDelayMs = 1500;
constexpr int kRequestTimeoutMs = 5000;
} // namespace

UpdateChecker::UpdateChecker(QWidget *dialogParent, QObject *parent)
    : QObject(parent), m_dialogParent(dialogParent),
      m_networkManager(new QNetworkAccessManager(this)) {}

QString UpdateChecker::currentVersion() {
#ifdef IMAGECONVERTER_VERSION
  return QStringLiteral(IMAGECONVERTER_VERSION);
#else
  return QStringLiteral("1.4.0");
#endif
}

bool UpdateChecker::isChecking() const { return m_isChecking; }

void UpdateChecker::checkIfWeeklyIntervalElapsed() {
  QSettings settings;
  const qint64 lastCheck =
      settings.value(QLatin1String(kSettingsLastCheckKey), 0).toLongLong();
  const qint64 now = QDateTime::currentSecsSinceEpoch();
  if (lastCheck > 0 && (now - lastCheck) < kWeeklyIntervalSecs) {
    return;
  }

  QTimer::singleShot(kAutoCheckStartupDelayMs, this,
                     [this]() { startCheck(false); });
}

void UpdateChecker::checkManually() { startCheck(true); }

void UpdateChecker::startCheck(bool manual) {
  if (m_isChecking || !m_networkManager) {
    return;
  }

  m_isChecking = true;
  emit checkingChanged(true);

  QNetworkRequest request(QUrl(QString::fromLatin1(kReleasesApiUrl)));
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("ImageConverter/%1").arg(currentVersion()));
  request.setRawHeader("Accept", "application/vnd.github+json");
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  request.setTransferTimeout(kRequestTimeoutMs);
#endif

  QNetworkReply *reply = m_networkManager->get(request);
  if (!reply) {
    m_isChecking = false;
    emit checkingChanged(false);
    return;
  }

  connect(reply, &QNetworkReply::finished, this,
          [this, reply, manual]() { handleReply(reply, manual); });
}

void UpdateChecker::handleReply(QNetworkReply *reply, bool manual) {
  if (reply) {
    reply->deleteLater();
  }

  m_isChecking = false;
  emit checkingChanged(false);

  if (!reply) {
    return;
  }

  const int httpStatus =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (reply->error() != QNetworkReply::NoError || httpStatus != 200) {
    if (manual && m_dialogParent) {
      ui::Toast::show(
          m_dialogParent, tr("Update Check Failed"),
          (httpStatus == 403)
              ? tr("GitHub API rate limit reached. Please try again later.")
              : tr("Unable to reach GitHub. Check your internet connection and "
                   "try again."),
          ui::ToastVariant::Warning);
    }
    return;
  }

  const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
  QString latestVersion;
  if (doc.isObject()) {
    latestVersion =
        doc.object().value(QStringLiteral("tag_name")).toString().trimmed();
    if (latestVersion.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
      latestVersion.remove(0, 1);
    }
    latestVersion = latestVersion.trimmed();
  }

  const QVersionNumber latestVer = QVersionNumber::fromString(latestVersion);
  const QVersionNumber currentVer =
      QVersionNumber::fromString(currentVersion());
  if (latestVer.isNull() || currentVer.isNull()) {
    if (manual && m_dialogParent) {
      ui::Toast::show(
          m_dialogParent, tr("Update Check Failed"),
          tr("Received an unexpected version format from GitHub."),
          ui::ToastVariant::Warning);
    }
    return;
  }

  QSettings settings;
  settings.setValue(QLatin1String(kSettingsLastCheckKey),
                    QDateTime::currentSecsSinceEpoch());

  if (QVersionNumber::compare(latestVer, currentVer) > 0) {
    if (!manual && (!m_dialogParent || !m_dialogParent->isVisible() ||
                    m_dialogParent->isMinimized() ||
                    QApplication::activeModalWidget() != nullptr)) {
      return;
    }
    showUpdateDialog(latestVersion);
  } else if (manual && m_dialogParent) {
    ui::Toast::show(
        m_dialogParent, tr("Up to Date"),
        tr("You are already using the latest version (%1).")
            .arg(currentVersion()),
        ui::ToastVariant::Success);
  }
}

void UpdateChecker::showUpdateDialog(const QString &latestVersion) {
  if (!m_dialogParent) {
    return;
  }

  ui::Dialog dialog(
      tr("Update Available"),
      tr("A new version (%1) of ImageConverter is available.\nYou are "
         "currently using version %2.\n\nOpen the download page now?")
          .arg(latestVersion, currentVersion()),
      m_dialogParent);
  dialog.setConfirmText(tr("Download Update"));
  dialog.setCancelText(tr("Later"));

  if (dialog.exec() == QDialog::Accepted) {
    QDesktopServices::openUrl(QUrl(QString::fromLatin1(kDownloadPageUrl)));
  }
}
