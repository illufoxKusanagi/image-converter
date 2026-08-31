#ifndef UI_AVATAR_H
#define UI_AVATAR_H

#include "../theme/theme.h"
#include <QPaintEvent>
#include <QPixmap>
#include <QWidget>

namespace ui {

enum class AvatarSize {
  Small,   // 28px
  Default, // 40px
  Large    // 56px
};

enum class AvatarStatus {
  None,
  Online,
  Offline,
  Busy,
  Away
};

class Avatar : public QWidget {
  Q_OBJECT

public:
  explicit Avatar(QWidget *parent = nullptr);
  explicit Avatar(const QString &initials, AvatarSize size = AvatarSize::Default, QWidget *parent = nullptr);
  explicit Avatar(const QPixmap &image, AvatarSize size = AvatarSize::Default, QWidget *parent = nullptr);
  ~Avatar() override = default;

  void setImage(const QPixmap &pixmap);
  void setInitials(const QString &initials);

  AvatarSize size() const;
  void setSize(AvatarSize size);

  AvatarStatus status() const;
  void setStatus(AvatarStatus status);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  QPixmap m_image;
  QString m_initials;
  AvatarSize m_size{AvatarSize::Default};
  AvatarStatus m_status{AvatarStatus::None};

  int diameter() const;
};

} // namespace ui

#endif // UI_AVATAR_H
