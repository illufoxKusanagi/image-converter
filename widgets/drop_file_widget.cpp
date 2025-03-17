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
                TextStyle::BodyBigBold() + "color : " + Colors::Grey900.name() +
                ";"
                "}"
                "QFrame[frameShape=\"4\"] {"
                "    background-color: " +
                Colors::Grey400.name() +
                ";"
                "    color: transparent;"
                "}");

  QLabel *icon = new QLabel(this);
  QLabel *label =
      new QLabel("Chose your file or drag your " + typeFile + " here...", this);
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
  label->setMinimumHeight(50);
  label->setMaximumWidth(400);
  label->setWordWrap(true);
  label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  mainLayout->addWidget(icon, 0, Qt::AlignCenter);
  mainLayout->addWidget(label, 0, Qt::AlignCenter);
  setupOrSeparatorLayout();
  mainLayout->addWidget(browseButton, 0, Qt::AlignCenter);
  setFixedSize(320, 320);
  setLayout(mainLayout);
}

void DropFileWidget::setupOrSeparatorLayout() {
  QString styling = "QLabel{"
                    "color: " +
                    Colors::Grey400.name() + ";" + TextStyle::BodySmallBold() +
                    "}";
  QWidget *orWidget = new QWidget(this);
  QHBoxLayout *orLayout = new QHBoxLayout(orWidget);
  QLabel *formatLabel = new QLabel("jpg, jpeg, png, webp", this);
  QLabel *optional = new QLabel("or", this);
  QFrame *lineAbove = new QFrame(this);
  QFrame *lineBelow = new QFrame(this);
  orLayout->setAlignment(Qt::AlignCenter);
  orLayout->setSpacing(8);
  lineAbove->setFrameShape(QFrame::HLine);
  lineAbove->setFrameShadow(QFrame::Plain);
  lineAbove->setFixedHeight(1);
  lineBelow->setFrameShape(QFrame::HLine);
  lineBelow->setFrameShadow(QFrame::Plain);
  lineBelow->setFixedHeight(1);
  formatLabel->setStyleSheet(styling);
  optional->setStyleSheet(styling);
  orLayout->addWidget(lineAbove, 1);
  orLayout->addWidget(optional, 0);
  orLayout->addWidget(lineBelow, 1);
  orLayout->setContentsMargins(40, 0, 40, 0);
  optional->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(formatLabel, 0, Qt::AlignCenter);
  mainLayout->addWidget(orWidget);
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