#ifndef UI_SELECT_H
#define UI_SELECT_H

#include "../theme/theme.h"
#include <QComboBox>

namespace ui {

class Select : public QComboBox {
  Q_OBJECT

public:
  explicit Select(QWidget *parent = nullptr);
  explicit Select(const QStringList &items, QWidget *parent = nullptr);
  ~Select() override = default;

private slots:
  void applyThemeStyles();
};

} // namespace ui

#endif // UI_SELECT_H
