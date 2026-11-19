#include "i18n.h"
#include <QCoreApplication>
#include <QSettings>

namespace ui {

I18n &I18n::instance() {
  static I18n s_instance;
  return s_instance;
}

I18n::I18n(QObject *parent) : QObject(parent) {}

Language I18n::language() const { return m_language; }

void I18n::setLanguage(Language lang) { applyLanguage(lang, true); }

void I18n::toggleLanguage() {
  setLanguage(m_language == Language::English ? Language::Indonesian
                                              : Language::English);
}

void I18n::loadSavedLanguage() {
  const QString saved =
      QSettings()
          .value(QStringLiteral("ui/language"), QStringLiteral("en"))
          .toString()
          .trimmed()
          .toLower();
  const Language lang =
      (saved == QStringLiteral("id")) ? Language::Indonesian : Language::English;
  applyLanguage(lang, false);
}

QString I18n::languageCode() const {
  return m_language == Language::Indonesian ? QStringLiteral("id")
                                            : QStringLiteral("en");
}

QString I18n::displayBadge() const {
  return m_language == Language::Indonesian ? QStringLiteral("ID")
                                            : QStringLiteral("EN");
}

void I18n::applyLanguage(Language lang, bool saveSetting) {
  const bool changed = (m_language != lang);

  if (saveSetting) {
    QSettings().setValue(QStringLiteral("ui/language"),
                         lang == Language::Indonesian ? QStringLiteral("id")
                                                      : QStringLiteral("en"));
  }

  if (QCoreApplication *app = QCoreApplication::instance()) {
    if (lang == Language::English) {
      if (m_translatorInstalled) {
        app->removeTranslator(&m_appTranslator);
        m_translatorInstalled = false;
      }
    } else if (changed || !m_translatorInstalled) {
      if (m_translatorInstalled) {
        app->removeTranslator(&m_appTranslator);
        m_translatorInstalled = false;
      }
      if (m_appTranslator.load(
              QStringLiteral(":/i18n/imageconverter_id.qm"))) {
        m_translatorInstalled = app->installTranslator(&m_appTranslator);
      }
    }
  }

  m_language = lang;
  if (changed) {
    emit languageChanged(m_language);
  }
}

} // namespace ui
