#ifndef UI_ALERT_H
#define UI_ALERT_H

#include <QFrame>
#include <QString>

class QLabel;
class QVBoxLayout;

namespace ui {

enum class AlertVariant { Info, Success, Warning, Destructive };

class Alert : public QFrame {
  Q_OBJECT

public:
  explicit Alert(QWidget *parent = nullptr);
  Alert(const QString &title, const QString &description,
        AlertVariant variant = AlertVariant::Info, QWidget *parent = nullptr);

  void setTitle(const QString &title);
  QString title() const;

  void setDescription(const QString &description);
  QString description() const;

  void setVariant(AlertVariant variant);
  AlertVariant variant() const;

private slots:
  void applyThemeStyles();

private:
  AlertVariant m_variant{AlertVariant::Info};
  QVBoxLayout *m_layout{nullptr};
  QLabel *m_variantLabel{nullptr};
  QLabel *m_titleLabel{nullptr};
  QLabel *m_descriptionLabel{nullptr};
};

} // namespace ui

#endif // UI_ALERT_H
