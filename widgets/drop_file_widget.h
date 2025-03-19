#ifndef DROPFILEWIDGET_H
#define DROPFILEWIDGET_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include "widgets/button_action.h"
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

class DropFileWidget : public QGroupBox {
  Q_OBJECT
public:
  explicit DropFileWidget(QWidget *parent = nullptr, QString typeFile = "File");

private:
  QVBoxLayout *mainLayout;
  void setupOrSeparatorLayout();
  QPixmap createColoredIcon(const QString &iconPath, const QColor &color,
                            int width, int height);
};

#endif // DROPFILEWIDGET_H
