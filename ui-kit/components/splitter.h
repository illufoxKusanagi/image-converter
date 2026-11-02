#ifndef UI_SPLITTER_H
#define UI_SPLITTER_H

#include <QSplitter>

namespace ui {

class Splitter : public QSplitter {
  Q_OBJECT

public:
  explicit Splitter(QWidget *parent = nullptr);
  explicit Splitter(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
  QSplitterHandle *createHandle() override;

private slots:
  void applyThemeStyles();
};

} // namespace ui

#endif // UI_SPLITTER_H
