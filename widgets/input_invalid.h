#ifndef INPUTINVALID_H
#define INPUTINVALID_H

#include "../styles/colors.h"
#include "../styles/text_style.h"
#include <QBoxLayout>
#include <QLabel>

class InputInvalid : public QWidget {
  Q_OBJECT
public:
  explicit InputInvalid(const QString &label = "", QWidget *parent = nullptr);

private:
  QLabel *m_invalidLabel;
};

#endif // INPUTINVALID_H
