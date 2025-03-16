#ifndef BUTTONSIDEBARACTIVE_H
#define BUTTONSIDEBARACTIVE_H

#include "button_sidebar_widget.h"
#include "resources/icon_type.h"
#include "styles/colors.h"
#include "styles/text_style.h"
#include <QBoxLayout>
#include <QPushButton>


class ButtonSidebarActive : public QPushButton {
  Q_OBJECT

public:
  explicit ButtonSidebarActive(const QString &iconType = "",
                               const QString &buttonLabel = "",
                               QWidget *parent = nullptr);
  void setLabelVisible(bool visible);
  void setIconVisible(bool visible);
  void setEnabled(bool isEnabled);
  void updateIcon(bool isHidden);
  void setIconSize(int width, int height);

protected:
  QString m_iconType;

private:
  static const QString s_styleHover;
  static const QString s_stylePressed;
  static const QString s_styleBase;

  QPushButton *buttonSidebarActive;
  ButtonSidebarWidget *m_contentWidget;
  QHBoxLayout *m_layout;
  bool isLabelVisible;
};

#endif // BUTTONSIDEBARACTIVE_H
