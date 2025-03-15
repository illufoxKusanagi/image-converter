#ifndef INPUTUPLOAD_H
#define INPUTUPLOAD_H

#include "../styles/colors.h"
#include "../styles/text_style.h"
#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>

class InputUpload : public QWidget {
  Q_OBJECT

public:
  explicit InputUpload(QWidget *parent = nullptr);

private:
  QPushButton *m_uploadButton;
  QLabel *m_uploadLabel;
};

#endif // INPUTUPLOAD_H
