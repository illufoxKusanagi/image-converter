#ifndef ICONPATHS_H
#define ICONPATHS_H

#include <QMap>
#include <QString>

class IconPaths {
public:
  static QString getIconPath(const QString &iconName) {
    static QMap<QString, QString> iconPaths = {
        {"run", ":/icons/icons/right-arrow-active.svg"},
        {"expand", ":/icons/icons/expand.svg"},
        {"collapse", ":/icons/icons/collapse.svg"},
        {"output", ":/icons/icons/play-active.svg"},
        {"outputDisabled", ":/icons/icons/play-disabled.svg"},
        {"categoryCollapsed", ":/icons/icons/category-active.svg"},
        {"category", ":/icons/icons/category-collapse.svg"},
        {"categoryCollapsedDisabled", ":/icons/icons/category-disabled.svg"},
        {"categoryDisabled", ":/icons/icons/category-collapse-disabled.svg"},
    };
    if (iconPaths.contains(iconName)) {
      return iconPaths.value(iconName);
    } else {
      return QString();
    }
  };
};

#endif // ICONPATHS_H
