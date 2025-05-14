#ifndef BUTTONSIDEBARWIDGET_H
#define BUTTONSIDEBARWIDGET_H

#include "resources/icon_type.h"
#include "styles/colors.h"
#include "styles/text_style.h"
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QSizePolicy>
#include <Qwidget>

/**
 * @brief Constructs a sidebar button widget with an optional icon and label.
 *
 * Initializes the widget with the specified parent, icon type, and button label. If no icon type or label is provided, the widget is created with default empty values.
 *
 * @param parent Optional parent widget.
 * @param iconType Optional icon type identifier.
 * @param buttonLabel Optional text label for the button.
 */
class ButtonSidebarWidget : public QWidget {
public:
  explicit ButtonSidebarWidget(QWidget *parent = nullptr,
                               const QString &iconType = "",
                               const QString &buttonLabel = "");
  void setLabelVisible(bool visible);
  void setIconVisible(bool visible);
  void setIconSize(int width, int height);
  void updateIcon(bool isEnabled);
  void setEnabled(bool isEnabled);
  void updateAppearance(bool isShown);

private:
  QLabel *m_iconLabel = nullptr;
  QLabel *m_textLabel = nullptr;
  QBoxLayout *mainLayout = nullptr;
  QString m_iconType;
  bool m_isEnabled = false;
  int m_fixedHeight;
  int m_fixedWidth;
  const int MINIMUM_SIZE = 32;
};

#endif // BUTTONSIDEBARWIDGET_H
