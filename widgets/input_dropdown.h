#ifndef INPUTDROPDOWN_H
#define INPUTDROPDOWN_H

#include "styles/colors.h"
#include "styles/text_style.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QOverload>
#include <QStringList>

/**
 * @brief Constructs an InputDropdown widget with optional parent and dropdown options.
 *
 * Initializes the dropdown with the provided list of string options. If no options are given, the dropdown will be empty.
 *
 * @param parent Optional parent widget.
 * @param options List of strings to populate the dropdown menu.
 */
class InputDropdown : public QWidget {
  Q_OBJECT

signals:
  void valueChanged();

public:
  explicit InputDropdown(QWidget *parent = nullptr,
                         const QStringList options = {});

  QString currentText() const;
  void setCurrentText(const QString &text);
  void setSize(int width, int height);
  double getValue();

private:
  QComboBox *m_dropdown;
  const QStringList m_options;
  void setValue();
};

#endif // INPUTDROPDOWN_H
