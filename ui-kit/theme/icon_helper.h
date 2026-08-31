#ifndef UI_ICON_HELPER_H
#define UI_ICON_HELPER_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace ui {

class Icon {
public:
  static QPixmap pixmap(const QString &resourcePath, const QColor &color, const QSize &size = QSize(20, 20));
  static QPixmap pixmap(const QString &resourcePath, const QColor &color, int width, int height);
  static QIcon get(const QString &resourcePath, const QColor &color, const QSize &size = QSize(20, 20));
};

} // namespace ui

#endif // UI_ICON_HELPER_H

