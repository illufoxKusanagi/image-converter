
#include "input_invalid.h"

InputInvalid::InputInvalid(const QString &label, QWidget *parent)
    : QWidget(parent),
      m_invalidLabel(new QLabel(" \'" + label +
                                    "\' input type isn't allowed. Use "
                                    "\'dropdown\', \'field\', or \'upload\'",
                                this)) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  m_invalidLabel->setStyleSheet(TextStyle::BodySmallBold() +
                                "color: " + Colors::Danger600.name() + ";");
  layout->addWidget(m_invalidLabel);
}
