#include "input_upload.h"

InputUpload::InputUpload(QWidget *parent)
    : QWidget(parent), m_uploadButton(new QPushButton("Choose csv File", this)),
      m_uploadLabel(new QLabel("No file selected", this)) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  m_uploadButton->setIcon(QIcon(":/icons/icons/upload.svg"));
  m_uploadButton->setIconSize(QSize(16, 16));

  layout->addWidget(m_uploadButton);
  layout->addWidget(m_uploadLabel);

  m_uploadButton->setStyleSheet("QPushButton{"
                                "min-width: 64px;"
                                "padding: 4px 12px; "
                                "border: 1px solid " +
                                Colors::Grey400.name() +
                                ";"
                                "border-radius: 8px;" +
                                TextStyle::BodySmallRegular() +
                                "color: " + Colors::Grey400.name() +
                                ";"
                                "}"
                                "QPushButton:pressed {"
                                "background-color: " +
                                Colors::Grey100.name() +
                                ";"
                                "}");
  m_uploadLabel->setStyleSheet(TextStyle::SubttileSmallRegular() +
                               "color: " + Colors::Grey700.name() + ";");
}