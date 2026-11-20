#include "mainwindow/update_checker.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>

namespace {
constexpr const char *kLatestReleaseUrl =
    "https://github.com/illufoxKusanagi/image-converter/releases/latest";
constexpr const char *kDownloadPageUrl =
    "https://ariefsatria.my.id/apps/image-converter";
constexpr const char *kSettingsLastCheckKey = "updates/lastCheckUtc";
constexpr qint64 kWeeklyIntervalSecs = 7LL * 24LL * 3600LL;
constexpr int kAutoCheckStartupDelayMs = 1500;
constexpr int kRequestTimeoutMs = 5000;

QString extractVersionFromReleaseUrl(const QUrl &url) {
  const QString path = url.path(QUrl::FullyDecoded);
  const QString marker = QStringLiteral("/releases/tag/");
  const int idx = path.indexOf(marker);
  if (idx < 0) {
    return {};
  }
  QString tag = path.mid(idx + marker.size()).trimmed();
  const int slashIdx = tag.indexOf(QLatin1Char('/'));
  if (slashIdx >= 0) {
    tag = tag.left(slashIdx);
  }
  if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
    tag.remove(0, 1);
  }
  return tag.trimmed();
}
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

  QNetworkRequest request(QUrl(QString::fromLatin1(kLatestReleaseUrl)));
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("ImageConverter/%1").arg(currentVersion()));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::ManualRedirectPolicy);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  request.setTransferTimeout(kRequestTimeoutMs);
#endif

  QNetworkReply *reply = m_networkManager->head(request);
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
  const bool isRedirect =
      (httpStatus == 301 || httpStatus == 302 || httpStatus == 303 ||
       httpStatus == 307 || httpStatus == 308);
  if (reply->error() != QNetworkReply::NoError ||
      (!isRedirect && httpStatus != 200)) {
    if (manual && m_dialogParent) {
      ui::Toast::show(
          m_dialogParent, tr("Update Check Failed"),
          tr("Unable to reach GitHub. Check your internet connection and "
             "try again."),
          ui::ToastVariant::Warning);
    }
    return;
  }

  QUrl targetUrl =
      reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
  if (!targetUrl.isValid()) {
    const QByteArray rawLocation = reply->rawHeader("Location").trimmed();
    if (!rawLocation.isEmpty()) {
      targetUrl = QUrl::fromEncoded(rawLocation);
    }
  }
  if (targetUrl.isValid() && targetUrl.isRelative()) {
    targetUrl = reply->url().resolved(targetUrl);
  }
  if (!targetUrl.isValid()) {
    targetUrl = reply->url();
  }

  const QString latestVersion = extractVersionFromReleaseUrl(targetUrl);

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
