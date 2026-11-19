#ifndef UI_I18N_H
#define UI_I18N_H

#include <QObject>
#include <QString>
#include <QTranslator>

namespace ui {

enum class Language {
  English = 0,
  Indonesian = 1,
};

class I18n : public QObject {
  Q_OBJECT

public:
  static I18n &instance();

  Language language() const;
  void setLanguage(Language lang);
  void toggleLanguage();
  void loadSavedLanguage();

  QString languageCode() const;
  QString displayBadge() const;

signals:
  void languageChanged(ui::Language lang);

private:
  explicit I18n(QObject *parent = nullptr);
  ~I18n() override = default;
  I18n(const I18n &) = delete;
  I18n &operator=(const I18n &) = delete;

  void applyLanguage(Language lang, bool saveSetting);

  Language m_language{Language::English};
  QTranslator m_appTranslator{this};
  bool m_translatorInstalled{false};
};

} // namespace ui

#endif // UI_I18N_H
