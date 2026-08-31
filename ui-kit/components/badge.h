#ifndef UI_BADGE_H
#define UI_BADGE_H

#include "../theme/theme.h"
#include <QLabel>

namespace ui {

enum class BadgeVariant {
  Default,
  Secondary,
  Outline,
  Destructive
};

class Badge : public QLabel {
  Q_OBJECT

public:
  explicit Badge(QWidget *parent = nullptr);
  explicit Badge(const QString &text, QWidget *parent = nullptr);
  Badge(const QString &text, BadgeVariant variant, QWidget *parent = nullptr);
  ~Badge() override = default;

  BadgeVariant variant() const;
  void setVariant(BadgeVariant variant);

private slots:
  void applyThemeStyles();

private:
  BadgeVariant m_variant{BadgeVariant::Default};
};

} // namespace ui

#endif // UI_BADGE_H
