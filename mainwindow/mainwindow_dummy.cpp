#include "mainwindow_dummy.h"

MainWindowDummy::MainWindowDummy(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Image Converter");
  resize(400, 200);

  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  label = new QLabel(
      "Seret dan lepas gambar di sini\natau klik tombol untuk memilih gambar.",
      this);
  label->setAlignment(Qt::AlignCenter);

  button = new QPushButton("Pilih Gambar", this);

  // Slider untuk kompresi JPG
  QLabel *qualityLabel = new QLabel("Kualitas JPG:", this);
  qualitySlider = new QSlider(Qt::Horizontal, this);
  qualitySlider->setRange(0, 100);
  qualitySlider->setValue(90);

  qualityValue = new QSpinBox(this);
  qualityValue->setRange(0, 100);
  qualityValue->setValue(90);

  // Sinkronisasi slider dengan spinbox
  connect(qualitySlider, &QSlider::valueChanged, qualityValue,
          &QSpinBox::setValue);
  connect(qualityValue, QOverload<int>::of(&QSpinBox::valueChanged),
          qualitySlider, &QSlider::setValue);

  // Layout
  layout->addWidget(label);
  layout->addWidget(button);

  QHBoxLayout *qualityLayout = new QHBoxLayout();
  qualityLayout->addWidget(qualityLabel);
  qualityLayout->addWidget(qualitySlider);
  qualityLayout->addWidget(qualityValue);
  layout->addLayout(qualityLayout);

  connect(button, &QPushButton::clicked, this, &MainWindowDummy::convertImage);

  // Mengaktifkan drag & drop
  setAcceptDrops(true);
}

void MainWindowDummy::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void MainWindowDummy::dropEvent(QDropEvent *event) {
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty())
    return;

  QString filePath = urls.first().toLocalFile();
  processImage(filePath);
}

void MainWindowDummy::convertImage() {
  QString filePath = QFileDialog::getOpenFileName(
      this, "Pilih Gambar", "", "Images (*.png *.jpg *.jpeg)");
  if (filePath.isEmpty())
    return;

  processImage(filePath);
}

void MainWindowDummy::processImage(const QString &filePath) {
  QImage img(filePath);
  if (img.isNull()) {
    QMessageBox::warning(this, "Error", "Gagal membuka gambar!");
    return;
  }

  QString outputPath;
  int quality = qualityValue->value(); // Ambil nilai kompresi dari slider

  if (filePath.endsWith(".png", Qt::CaseInsensitive)) {
    outputPath = filePath;
    outputPath.replace(".png", ".jpg");
    img.save(outputPath, "JPG", quality);
  } else if (filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
             filePath.endsWith(".jpeg", Qt::CaseInsensitive)) {
    outputPath = filePath;
    outputPath.replace(".jpg", ".png").replace(".jpeg", ".png");
    img.save(outputPath, "PNG");
  } else {
    QMessageBox::warning(this, "Error", "Format gambar tidak didukung!");
    return;
  }

  QMessageBox::information(this, "Sukses", "Gambar dikonversi: " + outputPath);
}
