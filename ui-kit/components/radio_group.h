#ifndef UI_RADIO_GROUP_H
#define UI_RADIO_GROUP_H

#include <QRadioButton>
#include <QStringList>
#include <QWidget>

class QBoxLayout;
class QButtonGroup;

namespace ui {

class RadioGroup : public QWidget {
  Q_OBJECT

public:
  explicit RadioGroup(QWidget *parent = nullptr);
  explicit RadioGroup(const QStringList &options, QWidget *parent = nullptr);

  // Negative IDs are assigned the first unused nonnegative ID. Duplicates are
  // ignored.
  void addOption(const QString &text, int id = -1);
  int checkedId() const;
  // -1 clears the selection; unknown IDs leave it unchanged.
  void setCheckedId(int id);
  QRadioButton *button(int id) const;

  Qt::Orientation orientation() const;
  void setOrientation(Qt::Orientation orientation);

signals:
  void selectionChanged(int id);

private slots:
  void applyThemeStyles();

private:
  QButtonGroup *m_group;
  QBoxLayout *m_layout;
  Qt::Orientation m_orientation{Qt::Vertical};
};

} // namespace ui

#endif // UI_RADIO_GROUP_H
