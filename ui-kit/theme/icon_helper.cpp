#include "icon_helper.h"
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

namespace ui {

QPixmap Icon::pixmap(const QString &resourcePath, const QColor &color, int width, int height) {
  QFile file(resourcePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return QPixmap();
  }

  QByteArray svgData = file.readAll();
  file.close();

  QString hexColor = color.name(QColor::HexRgb);
  // Replace currentColor or default black stroke/fill with requested theme color
  svgData.replace("currentColor", hexColor.toUtf8());
  svgData.replace("stroke=\"#000000\"", ("stroke=\"" + hexColor + "\"").toUtf8());
  svgData.replace("stroke=\"#000\"", ("stroke=\"" + hexColor + "\"").toUtf8());

  QSvgRenderer renderer(svgData);
  if (!renderer.isValid()) {
    return QPixmap();
  }

  QPixmap pixmap(width, height);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  renderer.render(&painter, QRectF(0, 0, width, height));
  painter.end();

  return pixmap;
}

QPixmap Icon::pixmap(const QString &resourcePath, const QColor &color, const QSize &size) {
  return pixmap(resourcePath, color, size.width(), size.height());
}

QIcon Icon::get(const QString &resourcePath, const QColor &color, const QSize &size) {
  return QIcon(pixmap(resourcePath, color, size));
}

} // namespace ui

