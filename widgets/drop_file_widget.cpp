#include "drop_file_widget.h"

DropFileWidget::DropFileWidget(QWidget *parent, QString typeFile)
    : QGroupBox(parent) {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(16);
  mainLayout->setAlignment(Qt::AlignCenter);
  setStyleSheet("QGroupBox {"
                "border: 2px solid " +
                Colors::Secondary400.name() +
                ";"
                "border-radius: 12px;"
                "}"
                "QLabel {"
                " background-color: transparent; " +
                TextStyle::BodyMediumRegular() +
                "color : " + Colors::Grey900.name() +
                ";"
                "}"
                "QFrame[frameShape=\"4\"] {"
                "    background-color: " +
                Colors::Grey400.name() +
                ";"
                "    color: transparent;"
                "    max-height: 0.02px;"
                "}");

  QLabel *icon = new QLabel(this);
  QLabel *label =
      new QLabel("drag and drop your " + typeFile + " here...", this);
  QPixmap pixmap(":/icons/icons/upload.svg");
  QPixmap coloredIcon =
      createColoredIcon(":/icons/icons/upload.svg", Colors::Primary600, 40, 40);
  QPixmap scaledPixmap = pixmap.scaled(40, 40, Qt::KeepAspectRatio);
  ButtonAction *browseButton = new ButtonAction("Browse", "", this);
  browseButton->setFixedSize(128, 40);
  browseButton->setEnabled(true);

  icon->setPixmap(coloredIcon);
  icon->setAlignment(Qt::AlignCenter);
  label->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(icon, 0, Qt::AlignCenter);
  mainLayout->addWidget(label, 0, Qt::AlignCenter);
  setupOrSeparatorLayout();
  mainLayout->addWidget(browseButton, 0, Qt::AlignCenter);
  setLayout(mainLayout);
}

void DropFileWidget::setupOrSeparatorLayout() {
  QLabel *optional = new QLabel("or", this);

  QFrame *lineAbove = new QFrame(this);
  lineAbove->setFrameShape(QFrame::HLine);
  lineAbove->setFixedHeight(1);

  QFrame *lineBelow = new QFrame(this);
  lineBelow->setFrameShape(QFrame::HLine);
  lineBelow->setFixedHeight(1);

  QHBoxLayout *orLayout = new QHBoxLayout();
  orLayout->setSpacing(8);
  orLayout->addWidget(lineAbove, 1);
  orLayout->addWidget(optional, 0);
  orLayout->addWidget(lineBelow, 1);
  orLayout->setContentsMargins(40, 0, 40, 0);
  optional->setAlignment(Qt::AlignCenter);
  mainLayout->addLayout(orLayout);
}

QPixmap DropFileWidget::createColoredIcon(const QString &iconPath,
                                          const QColor &color, int width,
                                          int height) {
  QPixmap originalPixmap(iconPath);
  if (originalPixmap.isNull()) {
    return QPixmap();
  }
  originalPixmap = originalPixmap.scaled(width, height, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
  QPixmap coloredPixmap(originalPixmap.size());
  coloredPixmap.fill(Qt::transparent);
  QPainter painter(&coloredPixmap);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.drawPixmap(0, 0, originalPixmap);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(coloredPixmap.rect(), color);
  painter.end();

  return coloredPixmap;
}