# AI Agent Master Fix Task & Full Replacement Code (`ImageConverter`)

> **How to use this file**: Copy the **Master Prompt** in Section 1 and paste it directly to your AI coding agent alongside this file (or feed each Task Step in Section 2 one by one if your model has a small context window). Every code block in Section 3 is a **100% complete, full-file replacement**—the AI agent does **not** need to write or invent any logic. It only needs to overwrite each listed file with the exact contents provided below.

## 1. Copy-Paste Master Prompt for Your AI Agent

```
You are executing an explicit, authorized multi-file bug fix task on the ImageConverter repository.
Do NOT ask clarifying questions, do NOT invent code, do NOT use placeholders or "// ... existing code ...", and do NOT skip any part of a file.

Your exact scope of work is to overwrite the following 10 files with the exact, complete C++ / CMake source code provided in Section 3 of `AI_AGENT_FIX_PROMPT.md`:
1. `main.cpp`
2. `mainwindow/mainwindow.h`
3. `mainwindow/mainwindow.cpp`
4. `widgets/drop_file_widget.cpp`
5. `pages/main_page.cpp`
6. `pages/pdf_page.h`
7. `pages/pdf_page.cpp`
8. `CMakeLists.txt`
9. `ui-kit/components/table.h`
10. `ui-kit/components/table.cpp`

Follow these steps in strict order:
STEP 1: Overwrite each of the 10 files above using the exact full-file code block from Section 3 of `AI_AGENT_FIX_PROMPT.md`.
STEP 2: Verify file integrity by checking that all 10 files exist on disk and have a byte size > 0 (`Length > 0`).
STEP 3: Compile the project using `cmake --build build` (or configure with `cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release` first if `build/` does not exist) and verify exit code `0`.
STEP 4: Report the file byte sizes and raw build output, then stop calling tools immediately.

```

## 2. Summary of Bugs Fixed by These Replacements

| File Path | What Was Broken | How the Replacement Code Fixes It | 
| ----- | ----- | ----- | 
| `main.cpp` | Hardcoded `app.setStyleSheet("QWidget { background-color: white; color: black;}")` overrode `ui::Theme` dark/light mode across child widgets. | Removes the global stylesheet override so `ui::Theme` tokens control Light, Dark, and System modes cleanly. | 
| `mainwindow/mainwindow.h` & `mainwindow/mainwindow.cpp` | `PdfPage` ("Compress PDF") was missing from `MainWindow`—only `MainPage` and `GalleryPage` were added to `m_tabs`. Tab icons also did not re-tint on theme toggle. | Adds `#include "pages/pdf_page.h"`, instantiates `m_pdfPage`, registers the **"Compress PDF"** tab at Index 1, and updates tab icons dynamically in `applyThemeStyles()`. | 
| `widgets/drop_file_widget.cpp` | 1\. `saveImage()` did not append `"." + format`, causing all single/batch image conversions and PDF extractions in `MainPage` to write extensionless files and report `0 B` size.  2\. `dropEvent()` and `onBrowseButtonPressed()` rejected `.pdf` files when `m_typeFile == "Image"`, making `MainPage`'s PDF-to-Image extraction unreachable. | 1\. Ensures `saveImage()` appends `"." + format` whenever `outputPath` does not already end with that extension.  2\. Allows `.pdf` files in `DropFileWidget` when `m_typeFile == "Image"` so dropping/browsing a PDF on the Convert Image tab extracts its pages to images. | 
| `pages/main_page.cpp` | In Scenario 1 (`Images -> PDF`), the quality slider only altered `dpi` (`72–300`) and scaled page point size (`width * 72.0 / dpi`), leaving the embedded `QImage` completely uncompressed. | Keeps physical PDF page dimensions accurate while downscaling and JPEG-compressing each image in memory according to the `0–100` quality slider before drawing to `QPdfWriter`. | 
| `pages/pdf_page.h` & `pages/pdf_page.cpp` | 1\. Used legacy `InputWidget`/`InputDropdown` where `setValue()` was an empty stub `{}` and `m_inputValue` was uninitialized.  2\. Used hardcoded light-mode colors (`Colors::*`).  3\. Full-page rasterization via `QPdfDocument` + `QPdfWriter` used broken scale math (`36–144 DPI`), applied noisy `Format_Indexed8` diffuse dithering that bloated `FlateDecode` (zlib) streams, and produced PDFs larger than the original. | 1\. Upgrades UI to `ui::Select`, `ui::FormField`, and `ui::Button` with full Dark/Light theme support and working preset ↔ slider sync.  2\. Adds a **Ghostscript (`pdfwrite`) primary engine** (matching PDF24 / iLovePDF / TinyPDF: preserves vector text/fonts, downsamples embedded images bicubically, and applies `/DCTEncode` JPEG compression).  3\. Provides a **corrected Pure-Qt fallback engine** when Ghostscript is absent (fixes DPI math, removes dithering, applies JPEG encoding, and enforces a strict size guard so output is never larger than the input PDF). | 
| `CMakeLists.txt` | The Qt 5 `else()` branch omitted `${PROJECT_WIDGETS}`, `${PROJECT_STYLES}`, and `${PROJECT_PAGES}`. | Includes all project source lists in both Qt 6 and Qt 5 build targets. | 
| `ui-kit/components/table.h` & `ui-kit/components/table.cpp` | Both files were `0 bytes` on disk, violating the repository's non-zero file size rule. | Implements a clean, theme-aware `ui::Table` component (`QTableWidget` subclass) so zero-byte files are eliminated. | 

## 3. Complete Replacement Files (Copy & Overwrite Verbatim)

### File 1 of 10: `main.cpp`

Overwrite `main.cpp` with the following complete code:

```
#include "mainwindow/mainwindow.h"
#include "ui-kit/theme/theme.h"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  QCoreApplication::setOrganizationName("Illufox Kusanagi");
  QCoreApplication::setApplicationName("Image Converter");

  QIcon appIcon;
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(16, 16));
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(32, 32));
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(64, 64));
  appIcon.addFile(":/icons/icons/icon-logo.png", QSize(128, 128));
  app.setWindowIcon(appIcon);

  MainWindow w;
  w.show();
  return app.exec();
}

```

### File 2 of 10: `mainwindow/mainwindow.h`

Overwrite `mainwindow/mainwindow.h` with the following complete code:

```
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pages/gallery_page.h"
#include "pages/main_page.h"
#include "pages/pdf_page.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/tabs.h"
#include "ui-kit/theme/theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

private slots:
  void applyThemeStyles();
  void onThemeToggleClicked();

private:
  ui::Tabs *m_tabs{nullptr};
  ui::Button *m_themeToggleBtn{nullptr};
  QLabel *m_titleLabel{nullptr};
  QWidget *m_centralWidget{nullptr};
  MainPage *m_mainPage{nullptr};
  PdfPage *m_pdfPage{nullptr};
  GalleryPage *m_galleryPage{nullptr};
};

#endif // MAINWINDOW_H

```

### File 3 of 10: `mainwindow/mainwindow.cpp`

Overwrite `mainwindow/mainwindow.cpp` with the following complete code:

```
#include "mainwindow.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setMinimumSize(460, 620);
  setWindowTitle("ImageConverter");

  m_centralWidget = new QWidget(this);
  QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);
  rootLayout->setContentsMargins(20, 18, 20, 20);
  rootLayout->setSpacing(18);

  // Header: Title & Theme Toggle
  QWidget *headerWidget = new QWidget(m_centralWidget);
  QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(0, 0, 0, 0);

  m_titleLabel = new QLabel("ImageConverter", headerWidget);
  m_titleLabel->setFont(ui::Theme::instance().typography().font(
      ui::Theme::instance().typography().sizeXl, QFont::Bold));

  m_themeToggleBtn = new ui::Button(
      ui::Theme::instance().isDark() ? "Light" : "Dark",
      ui::ButtonVariant::Ghost, ui::ButtonSize::Small, headerWidget);
  connect(m_themeToggleBtn, &QPushButton::clicked, this,
          &MainWindow::onThemeToggleClicked);

  headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
  headerLayout->addStretch(1);
  headerLayout->addWidget(m_themeToggleBtn, 0, Qt::AlignVCenter);
  rootLayout->addWidget(headerWidget);

  // Main Tabs: Convert Image, Compress PDF, UI Components
  m_tabs = new ui::Tabs(ui::TabVariant::Underline, m_centralWidget);

  m_mainPage = new MainPage(m_tabs);
  m_tabs->addTab("Convert Image",
                 ui::Icon::get(":/icons/icons/lucide-image.svg",
                               ui::Theme::instance().colors().foreground,
                               QSize(16, 16)),
                 m_mainPage);

  m_pdfPage = new PdfPage(m_tabs);
  m_tabs->addTab("Compress PDF",
                 ui::Icon::get(":/icons/icons/lucide-file-text.svg",
                               ui::Theme::instance().colors().foreground,
                               QSize(16, 16)),
                 m_pdfPage);

  m_galleryPage = new GalleryPage(m_tabs);
  m_tabs->addTab("UI Components",
                 ui::Icon::get(":/icons/icons/lucide-sliders.svg",
                               ui::Theme::instance().colors().foreground,
                               QSize(16, 16)),
                 m_galleryPage);

  rootLayout->addWidget(m_tabs, 1);
  setCentralWidget(m_centralWidget);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainWindow::applyThemeStyles);
  applyThemeStyles();
}

void MainWindow::onThemeToggleClicked() {
  ui::Theme::instance().toggleMode();
}

void MainWindow::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  ui::ThemeMode mode = ui::Theme::instance().mode();

  setStyleSheet(QString("QMainWindow { background-color: %1; }")
                    .arg(ui::StyleHelper::toHexString(c.background)));

  m_centralWidget->setStyleSheet(
      QString("background-color: %1;")
          .arg(ui::StyleHelper::toHexString(c.background)));

  m_titleLabel->setStyleSheet(
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground)));

  QString modeText;
  QString iconPath;
  if (mode == ui::ThemeMode::Light) {
    modeText = "Light";
    iconPath = ":/icons/icons/lucide-sun.svg";
  } else if (mode == ui::ThemeMode::Dark) {
    modeText = "Dark";
    iconPath = ":/icons/icons/lucide-moon.svg";
  } else {
    modeText = "System";
    iconPath = ":/icons/icons/lucide-monitor.svg";
  }

  m_themeToggleBtn->setText(modeText);
  m_themeToggleBtn->setIcon(
      ui::Icon::get(iconPath, c.foreground, QSize(16, 16)));
}

```

### File 4 of 10: `widgets/drop_file_widget.cpp`

Overwrite `widgets/drop_file_widget.cpp` with the following complete code:

```
#include "drop_file_widget.h"
#include "ui-kit/components/badge.h"
#include "ui-kit/components/separator.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QFontMetrics>
#include <QScrollBar>

DropFileWidget::DropFileWidget(QWidget *parent, QString typeFile,
                               SliderWidget *sliderWidget,
                               ImageExtension *sourceExtension)
    : QGroupBox(parent), m_sliderWidget(sliderWidget),
      m_sourceExtension(sourceExtension), m_qualityValue(100),
      m_typeFile(typeFile), m_isDragHovered(false) {
  setObjectName("DropFileWidget");
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(12, 12, 12, 12);
  mainLayout->setSpacing(8);
  mainLayout->setAlignment(Qt::AlignCenter);

  setupEmptyFileWidget();
  setupChosenFileWidget();
  setMinimumSize(420, 240);
  setMaximumHeight(260);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  setLayout(mainLayout);

  if (m_sliderWidget) {
    m_qualityValue = m_sliderWidget->getValue();
    connect(m_sliderWidget, &SliderWidget::valueChanged, this,
            &DropFileWidget::onSliderValueChanged);
  }
  connect(m_browseButton, &QPushButton::clicked, this,
          &DropFileWidget::onBrowseButtonPressed);
  setAcceptDrops(true);
  updateWidgetVisibility();

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &DropFileWidget::applyThemeStyles);
  applyThemeStyles();
}

void DropFileWidget::updateDropZoneStyle() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  QString borderColor = m_isDragHovered ? ui::StyleHelper::toHexString(c.primary)
                                        : ui::StyleHelper::toHexString(c.border);
  QString bgColor = m_isDragHovered ? ui::StyleHelper::toHexString(c.accent)
                                    : ui::StyleHelper::toHexString(c.card);
  setStyleSheet(QString("QGroupBox#DropFileWidget {"
                        "  border: 2px dashed %1;"
                        "  border-radius: %2px;"
                        "  background-color: %3;"
                        "}"
                        "QLabel {"
                        "  background-color: transparent;"
                        "}")
                    .arg(borderColor)
                    .arg(r.lg)
                    .arg(bgColor));
}

void DropFileWidget::setupEmptyFileWidget() {
  m_emptyFieldWidget = new QWidget(this);
  QVBoxLayout *emptyFieldLayout = new QVBoxLayout(m_emptyFieldWidget);
  emptyFieldLayout->setContentsMargins(0, 0, 0, 0);
  emptyFieldLayout->setSpacing(8);
  emptyFieldLayout->setAlignment(Qt::AlignCenter);

  m_icon = new QLabel(this);
  m_emptyLabel = new QLabel("Choose your " + m_typeFile + "(s) or drag here...", this);

  m_browseButton = new ui::Button("Browse Files", ui::ButtonVariant::Outline, ui::ButtonSize::Default, this);
  m_browseButton->setMinimumWidth(140);

  m_icon->setAlignment(Qt::AlignCenter);
  m_emptyLabel->setAlignment(Qt::AlignCenter);
  m_emptyLabel->setWordWrap(true);

  emptyFieldLayout->addWidget(m_icon, 0, Qt::AlignCenter);
  emptyFieldLayout->addWidget(m_emptyLabel, 0, Qt::AlignCenter);
  setupOrSeparatorLayout(emptyFieldLayout);
  emptyFieldLayout->addWidget(m_browseButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_emptyFieldWidget);
}

void DropFileWidget::setupChosenFileWidget() {
  m_chosenFileWidget = new QWidget(this);
  QVBoxLayout *chosenFileLayout = new QVBoxLayout(m_chosenFileWidget);
  chosenFileLayout->setContentsMargins(4, 4, 4, 4);
  chosenFileLayout->setSpacing(6);

  QHBoxLayout *headerLayout = new QHBoxLayout();
  headerLayout->setContentsMargins(4, 0, 4, 0);
  m_chosenHeaderLabel = new QLabel("Selected Files", this);

  m_clearAllButton = new ui::Button("Clear All", ui::ButtonVariant::Ghost, ui::ButtonSize::Small, this);
  connect(m_clearAllButton, &QPushButton::clicked, this,
          &DropFileWidget::onClearAllPressed);

  headerLayout->addWidget(m_chosenHeaderLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);
  headerLayout->addWidget(m_clearAllButton, 0, Qt::AlignRight | Qt::AlignVCenter);
  chosenFileLayout->addLayout(headerLayout);

  m_fileScrollArea = new QScrollArea(this);
  m_fileScrollArea->setWidgetResizable(true);
  m_fileScrollArea->setFrameShape(QFrame::NoFrame);

  m_fileListContainer = new QWidget(m_fileScrollArea);
  m_fileListContainer->setStyleSheet("QWidget { background: transparent; }");
  m_fileListLayout = new QVBoxLayout(m_fileListContainer);
  m_fileListLayout->setContentsMargins(0, 0, 4, 0);
  m_fileListLayout->setSpacing(4);
  m_fileListLayout->setAlignment(Qt::AlignTop);
  m_fileScrollArea->setWidget(m_fileListContainer);
  chosenFileLayout->addWidget(m_fileScrollArea, 1);

  QHBoxLayout *footerLayout = new QHBoxLayout();
  footerLayout->setContentsMargins(0, 0, 0, 0);
  m_addMoreButton = new ui::Button("+ Add More Files", ui::ButtonVariant::Ghost, ui::ButtonSize::Small, this);
  connect(m_addMoreButton, &QPushButton::clicked, this,
          &DropFileWidget::onBrowseButtonPressed);
  footerLayout->addStretch();
  footerLayout->addWidget(m_addMoreButton);
  footerLayout->addStretch();
  chosenFileLayout->addLayout(footerLayout);

  mainLayout->addWidget(m_chosenFileWidget);
  m_chosenFileWidget->setVisible(false);
}

void DropFileWidget::setupOrSeparatorLayout(QVBoxLayout *layout) {
  m_formatLabel = new QLabel(
      m_typeFile == "PDF"
          ? "Supports .pdf documents"
          : "Supports .png, .jpg, .jpeg, .webp, .tiff, .bmp, .gif, .pdf",
      this);
  m_formatLabel->setAlignment(Qt::AlignCenter);

  QWidget *orWidget = new QWidget(this);
  QHBoxLayout *orLayout = new QHBoxLayout(orWidget);
  orLayout->setAlignment(Qt::AlignCenter);
  orLayout->setSpacing(8);
  orLayout->setContentsMargins(40, 0, 40, 0);

  m_orLabel = new QLabel("or", this);
  m_orLabel->setAlignment(Qt::AlignCenter);

  ui::Separator *lineAbove = new ui::Separator(Qt::Horizontal, this);
  ui::Separator *lineBelow = new ui::Separator(Qt::Horizontal, this);

  orLayout->addWidget(lineAbove, 1);
  orLayout->addWidget(m_orLabel, 0);
  orLayout->addWidget(lineBelow, 1);

  layout->addWidget(m_formatLabel, 0, Qt::AlignCenter);
  layout->addWidget(orWidget);
}

void DropFileWidget::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  updateDropZoneStyle();

  if (m_emptyLabel) {
    m_emptyLabel->setFont(t.font(t.sizeBase, QFont::DemiBold));
    m_emptyLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                    .arg(ui::StyleHelper::toHexString(c.foreground)));
  }

  if (m_formatLabel) {
    m_formatLabel->setFont(t.font(t.sizeXs, QFont::Normal));
    m_formatLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                     .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_orLabel) {
    m_orLabel->setFont(t.font(t.sizeXs, QFont::Normal));
    m_orLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_chosenHeaderLabel) {
    m_chosenHeaderLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
    m_chosenHeaderLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                           .arg(ui::StyleHelper::toHexString(c.foreground)));
  }

  if (m_clearAllButton) {
    m_clearAllButton->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_clearAllButton->setStyleSheet(QString(
        "QPushButton {"
        "  background: transparent;"
        "  border: none;"
        "  color: %1;"
        "}"
        "QPushButton:hover {"
        "  text-decoration: underline;"
        "}")
        .arg(ui::StyleHelper::toHexString(c.destructive)));
  }

  if (m_fileScrollArea) {
    m_fileScrollArea->setStyleSheet(QString(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: transparent;"
        "  width: 6px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %1;"
        "  min-height: 20px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}")
        .arg(ui::StyleHelper::toHexString(c.border)));
  }

  if (m_browseButton) {
    m_browseButton->setIcon(ui::Icon::get(":/icons/icons/lucide-folder-open.svg", c.foreground, QSize(16, 16)));
  }

  if (m_addMoreButton) {
    m_addMoreButton->setIcon(ui::Icon::get(":/icons/icons/lucide-plus.svg", c.foreground, QSize(14, 14)));
  }

  QString iconRes = (m_typeFile == "PDF") ? ":/icons/icons/lucide-file-text.svg"
                                          : ":/icons/icons/lucide-upload-cloud.svg";
  QPixmap coloredIcon = ui::Icon::pixmap(iconRes, c.primary, 48, 48);
  if (m_icon) {
    m_icon->setPixmap(coloredIcon);
  }

  rebuildFileList();
}

void DropFileWidget::updateWidgetVisibility() {
  if (m_filePaths.isEmpty()) {
    m_emptyFieldWidget->setVisible(true);
    m_chosenFileWidget->setVisible(false);
  } else {
    m_emptyFieldWidget->setVisible(false);
    m_chosenFileWidget->setVisible(true);
    rebuildFileList();
  }
}

void DropFileWidget::rebuildFileList() {
  if (!m_fileListLayout) return;

  QLayoutItem *item;
  while ((item = m_fileListLayout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  qint64 totalBytes = 0;
  QPixmap pdfRowIcon = ui::Icon::pixmap(":/icons/icons/lucide-file-text.svg", c.primary, 18, 18);
  QPixmap imgRowIcon = ui::Icon::pixmap(":/icons/icons/lucide-image.svg", c.primary, 18, 18);

  for (int i = 0; i < m_filePaths.size(); ++i) {
    const QString &path = m_filePaths.at(i);
    QFileInfo fi(path);
    qint64 size = fi.size();
    totalBytes += size;
    bool isPdfFile = (fi.suffix().toLower() == "pdf");

    QWidget *card = new QWidget(m_fileListContainer);
    card->setFixedHeight(32);
    card->setStyleSheet(QString(
        "QWidget {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "}")
        .arg(ui::StyleHelper::toHexString(c.secondary))
        .arg(ui::StyleHelper::toHexString(c.border))
        .arg(r.sm));

    QHBoxLayout *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(8, 2, 6, 2);
    cardLayout->setSpacing(6);

    QLabel *iconLabel = new QLabel(card);
    iconLabel->setPixmap(isPdfFile ? pdfRowIcon : imgRowIcon);
    iconLabel->setFixedSize(18, 18);
    iconLabel->setStyleSheet("border: none; background: transparent;");

    QFontMetrics fm(font());
    QString elidedName = fm.elidedText(fi.fileName(), Qt::ElideMiddle, 210);
    QLabel *nameLabel = new QLabel(elidedName, card);
    nameLabel->setToolTip(fi.fileName());
    nameLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
    nameLabel->setStyleSheet(QString("border: none; background: transparent; color: %1;")
                                 .arg(ui::StyleHelper::toHexString(c.foreground)));

    ui::Badge *sizeBadge =
        new ui::Badge(formatFileSize(size), ui::BadgeVariant::Secondary, card);

    QPushButton *upBtn = new QPushButton(card);
    upBtn->setFixedSize(20, 20);
    upBtn->setCursor(Qt::PointingHandCursor);
    upBtn->setToolTip("Move up");
    upBtn->setEnabled(i > 0);
    upBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-chevron-up.svg", i > 0 ? c.mutedForeground : c.border, QSize(14, 14)));
    upBtn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(upBtn, &QPushButton::clicked, this, [this, i]() { moveFileUp(i); });

    QPushButton *downBtn = new QPushButton(card);
    downBtn->setFixedSize(20, 20);
    downBtn->setCursor(Qt::PointingHandCursor);
    downBtn->setToolTip("Move down");
    downBtn->setEnabled(i < m_filePaths.size() - 1);
    downBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-chevron-down.svg", i < m_filePaths.size() - 1 ? c.mutedForeground : c.border, QSize(14, 14)));
    downBtn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(downBtn, &QPushButton::clicked, this, [this, i]() { moveFileDown(i); });

    QPushButton *removeBtn = new QPushButton(card);
    removeBtn->setFixedSize(20, 20);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setToolTip("Remove file");
    removeBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-x.svg", c.mutedForeground, QSize(14, 14)));
    removeBtn->setStyleSheet("QPushButton { background: transparent; border: none; }");
    connect(removeBtn, &QPushButton::clicked, this, [this, i]() { removeFileAt(i); });

    cardLayout->addWidget(iconLabel);
    cardLayout->addWidget(nameLabel, 1);
    cardLayout->addWidget(sizeBadge);
    cardLayout->addWidget(upBtn);
    cardLayout->addWidget(downBtn);
    cardLayout->addWidget(removeBtn);

    m_fileListLayout->addWidget(card);
  }

  QString countStr = m_filePaths.size() == 1
                         ? "1 file selected"
                         : QString("%1 files selected").arg(m_filePaths.size());
  if (m_chosenHeaderLabel) {
    m_chosenHeaderLabel->setText(countStr + " • " + formatFileSize(totalBytes));
  }
}

void DropFileWidget::removeFileAt(int index) {
  if (index >= 0 && index < m_filePaths.size()) {
    m_filePaths.removeAt(index);
    updateWidgetVisibility();
  }
}

void DropFileWidget::moveFileUp(int index) {
  if (index > 0 && index < m_filePaths.size()) {
    m_filePaths.swapItemsAt(index, index - 1);
    rebuildFileList();
  }
}

void DropFileWidget::moveFileDown(int index) {
  if (index >= 0 && index < m_filePaths.size() - 1) {
    m_filePaths.swapItemsAt(index, index + 1);
    rebuildFileList();
  }
}

void DropFileWidget::onClearAllPressed() {
  clearFiles();
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

void DropFileWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    m_isDragHovered = true;
    updateDropZoneStyle();
    event->acceptProposedAction();
  }
}

void DropFileWidget::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void DropFileWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  m_isDragHovered = false;
  updateDropZoneStyle();
  event->accept();
}

void DropFileWidget::dropEvent(QDropEvent *event) {
  m_isDragHovered = false;
  updateDropZoneStyle();
  const QMimeData *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    QList<QUrl> urls = mimeData->urls();
    QStringList newPaths;
    for (const QUrl &url : urls) {
      if (url.isLocalFile()) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        QString ext = fileInfo.suffix().toLower();
        if (m_typeFile == "PDF") {
          if (ext == "pdf") {
            newPaths.append(QDir::cleanPath(filePath));
          }
        } else {
          QStringList validExts = {"png", "jpg", "jpeg", "webp", "tiff", "bmp", "gif", "pdf"};
          if (validExts.contains(ext)) {
            newPaths.append(QDir::cleanPath(filePath));
          }
        }
      }
    }
    for (const QString &np : newPaths) {
      if (!m_filePaths.contains(np)) {
        m_filePaths.append(np);
      }
    }
    updateWidgetVisibility();
  }
}

QString DropFileWidget::getFilePath() const {
  return m_filePaths.isEmpty() ? QString() : m_filePaths.first();
}

QStringList DropFileWidget::getFilePaths() const {
  return m_filePaths;
}

void DropFileWidget::clearFiles() {
  m_filePaths.clear();
  updateWidgetVisibility();
}

QString DropFileWidget::imageExtensionToString(
    const ImageExtension &sourceExtension) const {
  switch (sourceExtension) {
  case JPG:
    return "jpg";
  case JPEG:
    return "jpeg";
  case PNG:
    return "png";
  case WEBP:
    return "webp";
  case TIFF:
    return "tiff";
  case BMP:
    return "bmp";
  case GIF:
    return "gif";
  case PDF:
    return "pdf";
  default:
    return "";
  }
}

QString DropFileWidget::formatFileSize(qint64 bytes) {
  if (bytes < 1024) {
    return QString("%1 B").arg(bytes);
  } else if (bytes < 1024 * 1024) {
    return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
  } else {
    return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
  }
}

bool DropFileWidget::saveImage(const QImage *image, const QString &outputPath,
                               const int quality,
                               const ImageExtension *sourceExtension) {
  if (!image || image->isNull() || !sourceExtension) {
    return false;
  }
  QString format = imageExtensionToString(*sourceExtension).toLower();
  if (format.isEmpty()) {
    return false;
  }
  int effectiveQuality = (*sourceExtension == PNG) ? -1 : quality;
  QString finalPath = outputPath;
  if (!finalPath.endsWith("." + format, Qt::CaseInsensitive)) {
    finalPath += "." + format;
  }
  return image->save(finalPath, format.toUpper().toUtf8().constData(),
                     effectiveQuality);
}

void DropFileWidget::convertImage(const QString &sourcePath) {
  if (sourcePath.isEmpty() || !m_sourceExtension) {
    return;
  }
  QImage img(sourcePath);
  if (img.isNull()) {
    return;
  }
  QFileInfo fi(sourcePath);
  QString targetExtStr = imageExtensionToString(*m_sourceExtension).toLower();
  QString defaultPath = fi.dir().filePath(fi.completeBaseName() + "." + targetExtStr);
  QString filter = QString("%1 Files (*.%2);;All Files (*)").arg(targetExtStr.toUpper(), targetExtStr);
  QString outputPath = QFileDialog::getSaveFileName(this, "Save Converted Image", defaultPath, filter);
  if (outputPath.isEmpty()) {
    return;
  }
  int quality = m_sliderWidget ? m_sliderWidget->getValue() : 100;
  saveImage(&img, outputPath, quality, m_sourceExtension);
}

void DropFileWidget::onBrowseButtonPressed() {
  QStringList fileNames;
  if (m_typeFile == "PDF") {
    fileNames =
        QFileDialog::getOpenFileNames(this, "Select PDF Files", QDir::homePath(),
                                      "PDF Files (*.pdf);;All Files (*)");
  } else {
    fileNames = QFileDialog::getOpenFileNames(
        this, "Select Image or PDF Files", QDir::homePath(),
        "Supported Files (*.png *.jpg *.jpeg *.webp *.tiff *.bmp *.gif *.pdf);;"
        "Image Files (*.png *.jpg *.jpeg *.webp *.tiff *.bmp *.gif);;"
        "PDF Files (*.pdf);;All Files (*)");
  }
  if (fileNames.isEmpty()) {
    return;
  }
  for (const QString &fn : fileNames) {
    QString clean = QDir::cleanPath(QDir::fromNativeSeparators(fn));
    if (!m_filePaths.contains(clean)) {
      m_filePaths.append(clean);
    }
  }
  updateWidgetVisibility();
}

void DropFileWidget::onSliderValueChanged() {
  if (m_sliderWidget) {
    m_qualityValue = m_sliderWidget->getValue();
  }
}

```

### File 5 of 10: `pages/main_page.cpp`

Overwrite `pages/main_page.cpp` with the following complete code:

```
#include "main_page.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QBuffer>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QtConcurrent/QtConcurrent>

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)),
      m_targetImageExtension(DropFileWidget::ImageExtension::JPG) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(14);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  if (m_targetExtensionSelect) {
    onImageTargetExtensionChanged();
  }
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this, &MainPage::applyThemeStyles);
  applyThemeStyles();
}

MainPage::~MainPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"JPG", "JPEG", "PNG", "WEBP", "TIFF", "BMP", "GIF", "PDF"};
  m_targetExtensionSelect = new ui::Select(extensionOptions, this);
  m_targetExtensionField = new ui::FormField("Target Format", m_targetExtensionSelect, this);
  connect(m_targetExtensionSelect, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onImageTargetExtensionChanged);
}

void MainPage::setupImageAttribute() {
  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_targetExtensionField, 1, Qt::AlignTop);
  mainLayout->addLayout(attributeLayout);
}

void MainPage::setupImageLayout() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_dragWidget =
      new DropFileWidget(this, "Image", m_qualitySlider, &m_targetImageExtension);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setFixedHeight(22);
  m_progressBar->setAlignment(Qt::AlignCenter);
  m_progressBar->setTextVisible(true);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Convert Files", ui::ButtonVariant::Primary, ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton = new ui::Button("Cancel Conversion", ui::ButtonVariant::Destructive, ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &MainPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  setupImageAttribute();
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
}

void MainPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  m_progressBar->setFont(t.font(t.sizeXs, QFont::DemiBold));
  m_progressBar->setStyleSheet(QString(
      "QProgressBar {"
      "  border: 1px solid %1;"
      "  border-radius: %2px;"
      "  text-align: center;"
      "  background-color: %3;"
      "  color: %4;"
      "}"
      "QProgressBar::chunk {"
      "  background-color: %5;"
      "  border-radius: %6px;"
      "}")
      .arg(ui::StyleHelper::toHexString(c.border))
      .arg(r.md)
      .arg(ui::StyleHelper::toHexString(c.muted))
      .arg(ui::StyleHelper::toHexString(c.foreground))
      .arg(ui::StyleHelper::toHexString(c.primary))
      .arg(r.md > 2 ? r.md - 1 : 2));

  if (m_processButton) {
    m_processButton->setIcon(ui::Icon::get(":/icons/icons/lucide-refresh-cw.svg", c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void MainPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  m_progressBar->setVisible(isProcessing);
  m_dragWidget->setEnabled(!isProcessing);
  if (isProcessing) {
    m_qualitySlider->setEnabled(false);
  } else {
    onImageTargetExtensionChanged();
  }
}

void MainPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setText("Cancelling...");
    m_cancelButton->setEnabled(false);
  }
}

void MainPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    MessageBoxWidget messageBox("Error", "Please select at least one file.",
                                MessageBoxWidget::Warning, this);
    messageBox.exec();
    return;
  }

  DropFileWidget::ImageExtension targetExt = m_targetImageExtension;
  QString targetFormatString = m_dragWidget->imageExtensionToString(targetExt);
  int quality = m_qualitySlider->getValue();

  // SCENARIO 1: Images -> PDF (Single or Merged Multi-Page)
  if (targetExt == DropFileWidget::ImageExtension::PDF) {
    QString suggestedName;
    if (sourcePaths.size() == 1) {
      suggestedName = QFileInfo(sourcePaths.first())
                          .dir()
                          .filePath(QFileInfo(sourcePaths.first()).baseName() + ".pdf");
    } else {
      suggestedName = QFileInfo(sourcePaths.first()).dir().filePath("merged_images.pdf");
    }

    QString outputPath = QFileDialog::getSaveFileName(
        this, sourcePaths.size() == 1 ? "Save PDF" : "Save Merged PDF",
        suggestedName, "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0);
    m_progressBar->setFormat(sourcePaths.size() == 1 ? "Converting to PDF... %p%"
                                                     : "Merging into PDF... %p%");

    QFuture<void> future =
        QtConcurrent::run([this, sourcePaths, outputPath, quality, totalFiles]() {
          QString tempOutputPath = outputPath + ".tmp";
          int clampedQuality = qBound(1, quality, 100);
          int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
          qreal scaleFactor = 0.35 + (0.65 * clampedQuality) / 100.0;

          QPdfWriter pdfWriter(tempOutputPath);
          pdfWriter.setResolution(targetDpi);
          pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

          qint64 totalOrigBytes = 0;
          int successCount = 0;
          int skippedCount = 0;
          bool ok = true;

          QPainter painter;
          for (int i = 0; i < totalFiles; ++i) {
            if (m_isCancelled) {
              ok = false;
              break;
            }

            const QString &path = sourcePaths.at(i);
            QImage origImage(path);
            if (origImage.isNull()) {
              skippedCount++;
              QMetaObject::invokeMethod(
                  this, [this, i]() { m_progressBar->setValue(i + 1); },
                  Qt::QueuedConnection);
              continue;
            }

            // Preserve consistent physical page dimensions in points (at 96 base DPI)
            QSizeF ptSize(origImage.width() * 72.0 / 96.0,
                          origImage.height() * 72.0 / 96.0);
            pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

            // Downscale & compress image according to quality slider
            QImage processedImg = origImage.convertToFormat(QImage::Format_RGB888);
            if (clampedQuality < 95) {
              QSize scaledSize(
                  qMax(1, qRound(origImage.width() * scaleFactor)),
                  qMax(1, qRound(origImage.height() * scaleFactor)));
              processedImg = processedImg.scaled(scaledSize, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
              QByteArray jpegBytes;
              QBuffer buffer(&jpegBytes);
              if (buffer.open(QIODevice::WriteOnly)) {
                processedImg.save(&buffer, "JPG", clampedQuality);
                buffer.close();
                QImage reloaded;
                if (reloaded.loadFromData(jpegBytes, "JPG")) {
                  processedImg = reloaded;
                }
              }
            }

            if (successCount == 0) {
              if (!painter.begin(&pdfWriter)) {
                ok = false;
                break;
              }
              painter.setRenderHint(QPainter::SmoothPixmapTransform);
              painter.setRenderHint(QPainter::Antialiasing);
            } else {
              if (!pdfWriter.newPage()) {
                ok = false;
                break;
              }
            }

            QRect pageRect = pdfWriter.pageLayout().paintRectPixels(targetDpi);
            painter.drawImage(pageRect, processedImg);

            totalOrigBytes += QFileInfo(path).size();
            successCount++;

            QMetaObject::invokeMethod(
                this,
                [this, i]() {
                  m_progressBar->setValue(i + 1);
                },
                Qt::QueuedConnection);
          }

          if (painter.isActive()) {
            painter.end();
          }

          if (!ok || m_isCancelled || successCount == 0) {
            QFile::remove(tempOutputPath);
          } else {
            const bool hadExisting = QFile::exists(outputPath);
            const QString backupPath = outputPath + ".bak";
            if (hadExisting) {
              QFile::remove(backupPath);
              if (!QFile::rename(outputPath, backupPath)) {
                QFile::remove(tempOutputPath);
                ok = false;
              }
            }
            if (ok && !QFile::rename(tempOutputPath, outputPath)) {
              QFile::remove(tempOutputPath);
              if (hadExisting) {
                QFile::rename(backupPath, outputPath);
              }
              ok = false;
            }
            if (ok && hadExisting) {
              QFile::remove(backupPath);
            }
          }

          QMetaObject::invokeMethod(
              this,
              [this, outputPath, ok, successCount, skippedCount, totalOrigBytes]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                if (m_isCancelled) {
                  MessageBoxWidget messageBox(
                      "Cancelled", "PDF creation was cancelled.",
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else if (ok && successCount > 0) {
                  qint64 outSize = QFileInfo(outputPath).size();
                  QString sizeStats =
                      QString("\n\nTotal Original: %1\nPDF Size: %2")
                          .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                          .arg(DropFileWidget::formatFileSize(outSize));
                  QString msg =
                      (successCount == 1)
                          ? QString("Image converted to PDF successfully!%1")
                                .arg(sizeStats)
                          : QString("Successfully merged %1 images into PDF!%2")
                                .arg(successCount)
                                .arg(sizeStats);
                  if (skippedCount > 0) {
                    msg += QString("\n(%1 invalid image(s) skipped)").arg(skippedCount);
                  }
                  MessageBoxWidget messageBox("Success", msg,
                                              MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else {
                  MessageBoxWidget messageBox("Error", "Failed to create PDF!",
                                              MessageBoxWidget::Critical, this);
                  messageBox.exec();
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 2: Input contains PDF -> Extract PDF Pages into Images
  bool hasPdfInput = false;
  int estimatedTotalPages = 0;
  for (const QString &p : sourcePaths) {
    if (QFileInfo(p).suffix().toLower() == "pdf") {
      hasPdfInput = true;
      QPdfDocument doc;
      if (doc.load(p) == QPdfDocument::Error::None) {
        estimatedTotalPages += doc.pageCount();
      } else {
        estimatedTotalPages += 1;
      }
    } else {
      estimatedTotalPages += 1;
    }
  }

  if (hasPdfInput) {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory for Extracted Images", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    m_progressBar->setRange(0, estimatedTotalPages > 0 ? estimatedTotalPages : 1);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Extracting PDF pages (%v of %m)... %p%");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePaths, outputDir, quality, targetExt, dragWidget]() {
          int totalExtracted = 0;
          int failedCount = 0;
          int processedItems = 0;
          qint64 totalOutputBytes = 0;
          QSet<QString> usedBaseNames;
          QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

          for (const QString &sourcePath : sourcePaths) {
            if (m_isCancelled) break;

            if (QFileInfo(sourcePath).suffix().toLower() == "pdf") {
              QPdfDocument doc;
              if (doc.load(sourcePath) != QPdfDocument::Error::None) {
                failedCount++;
                processedItems++;
                QMetaObject::invokeMethod(
                    this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                    Qt::QueuedConnection);
                continue;
              }

              int pageCount = doc.pageCount();
              QString baseName = QFileInfo(sourcePath).baseName();

              for (int p = 0; p < pageCount; ++p) {
                if (m_isCancelled) break;

                QSizeF ptSize = doc.pagePointSize(p);
                int extractDpi = (quality <= 0) ? 72 : (72 + static_cast<int>((quality / 100.0) * (600 - 72)));
                qreal scale = extractDpi / 72.0;
                QSize renderSize(qMax(1, static_cast<int>(ptSize.width() * scale)),
                                 qMax(1, static_cast<int>(ptSize.height() * scale)));
                QImage pageImg = doc.render(p, renderSize);

                if (pageImg.isNull()) {
                  failedCount++;
                  processedItems++;
                  QMetaObject::invokeMethod(
                      this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                      Qt::QueuedConnection);
                  continue;
                }

                QString uniqueName = QString("%1_page%2").arg(baseName).arg(p + 1);
                int suffix = 1;
                while (usedBaseNames.contains(uniqueName.toLower()) ||
                       QFile::exists(
                           QDir(outputDir).filePath(uniqueName + "." + ext))) {
                  uniqueName =
                      QString("%1_page%2_%3").arg(baseName).arg(p + 1).arg(suffix++);
                }
                usedBaseNames.insert(uniqueName.toLower());

                QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
                if (dragWidget->saveImage(&pageImg, baseOutputPath, quality,
                                          &targetExt)) {
                  totalExtracted++;
                  totalOutputBytes +=
                      QFileInfo(baseOutputPath + "." + ext).size();
                } else {
                  failedCount++;
                }

                processedItems++;
                QMetaObject::invokeMethod(
                    this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                    Qt::QueuedConnection);
              }
            } else {
              QImage image(sourcePath);
              if (image.isNull()) {
                failedCount++;
                processedItems++;
                QMetaObject::invokeMethod(
                    this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                    Qt::QueuedConnection);
                continue;
              }
              QString baseName = QFileInfo(sourcePath).baseName();
              QString uniqueName = baseName;
              int suffix = 1;
              while (usedBaseNames.contains(uniqueName.toLower()) ||
                     QFile::exists(
                         QDir(outputDir).filePath(uniqueName + "." + ext))) {
                uniqueName = QString("%1_%2").arg(baseName).arg(suffix++);
              }
              usedBaseNames.insert(uniqueName.toLower());

              QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
              if (dragWidget->saveImage(&image, baseOutputPath, quality,
                                        &targetExt)) {
                totalExtracted++;
                totalOutputBytes +=
                    QFileInfo(baseOutputPath + "." + ext).size();
              } else {
                failedCount++;
              }

              processedItems++;
              QMetaObject::invokeMethod(
                  this, [this, processedItems]() { m_progressBar->setValue(processedItems); },
                  Qt::QueuedConnection);
            }
          }

          QMetaObject::invokeMethod(
              this,
              [this, totalExtracted, failedCount, totalOutputBytes, outputDir]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                if (m_isCancelled) {
                  MessageBoxWidget messageBox(
                      "Cancelled", "Extraction was cancelled.",
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else if (totalExtracted > 0) {
                  QString msg =
                      QString("Extracted %1 image(s) to:\n%2\n\nTotal Size: %3")
                          .arg(totalExtracted)
                          .arg(outputDir)
                          .arg(DropFileWidget::formatFileSize(totalOutputBytes));
                  if (failedCount > 0) {
                    msg += QString("\n(%1 failed)").arg(failedCount);
                  }
                  MessageBoxWidget messageBox("Success", msg,
                                              MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else {
                  MessageBoxWidget messageBox(
                      "Error", "Failed to extract images from PDF!",
                      MessageBoxWidget::Critical, this);
                  messageBox.exec();
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 3: Standard Image -> Image Conversion (Single & Batch)
  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    QString filter = targetFormatString.toUpper() + " (*." +
                     targetFormatString.toLower() + ")";
    QString suggestedName =
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_converted");

    QString outputPathWithExt = QFileDialog::getSaveFileName(
        this, "Save Image As", suggestedName, filter);

    if (outputPathWithExt.isEmpty()) {
      return;
    }

    const QString targetSuffix = targetFormatString.toLower();
    if (QFileInfo(outputPathWithExt).suffix().toLower() != targetSuffix) {
      outputPathWithExt += "." + targetSuffix;
    }

    QFileInfo outFi(outputPathWithExt);
    QString outputPathWithoutExt = outFi.dir().filePath(outFi.completeBaseName());

    setProcessingState(true);
    m_isCancelled = false;
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Converting image... %p%");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePath, outputPathWithoutExt, outputPathWithExt, quality,
         targetExt, targetFormatString, dragWidget]() {
          QImage image(sourcePath);
          bool success = false;
          if (!image.isNull() && !m_isCancelled) {
            success = dragWidget->saveImage(&image, outputPathWithoutExt,
                                            quality, &targetExt);
          }

          QMetaObject::invokeMethod(
              this,
              [this, sourcePath, success, outputPathWithExt,
               targetFormatString]() {
                m_progressBar->setValue(1);
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                if (m_isCancelled) {
                  MessageBoxWidget messageBox(
                      "Cancelled", "Image conversion was cancelled.",
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else if (success) {
                  qint64 origSize = QFileInfo(sourcePath).size();
                  qint64 outSize = QFileInfo(outputPathWithExt).size();
                  QString sizeStats;
                  if (origSize > 0 && outSize > 0) {
                    double diffPct =
                        (1.0 - (static_cast<double>(outSize) / origSize)) * 100.0;
                    sizeStats =
                        QString("\n\nOriginal: %1\nConverted: %2 (%3% %4)")
                            .arg(DropFileWidget::formatFileSize(origSize))
                            .arg(DropFileWidget::formatFileSize(outSize))
                            .arg(qAbs(diffPct), 0, 'f', 1)
                            .arg(diffPct >= 0 ? "smaller" : "larger");
                  }

                  MessageBoxWidget messageBox(
                      "Success",
                      QString("Image converted successfully to %1%2")
                          .arg(outputPathWithExt)
                          .arg(sizeStats),
                      MessageBoxWidget::Information, this);
                  messageBox.exec();
                } else {
                  MessageBoxWidget messageBox(
                      "Error",
                      QString("Failed to save image as %1")
                          .arg(targetFormatString),
                      MessageBoxWidget::Critical, this);
                  messageBox.exec();
                }
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  } else {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    setProcessingState(true);
    m_isCancelled = false;
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0);
    m_progressBar->setFormat(QString("Converting %v of %m images (%p%)"));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run(
        [this, sourcePaths, outputDir, quality, targetExt, totalFiles, dragWidget]() {
          int successCount = 0;
          int failureCount = 0;
          qint64 totalOrigBytes = 0;
          qint64 totalOutputBytes = 0;
          QSet<QString> usedBaseNames;
          QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

          for (int i = 0; i < totalFiles; ++i) {
            if (m_isCancelled) {
              break;
            }

            const QString &sourcePath = sourcePaths.at(i);
            QImage image(sourcePath);
            if (image.isNull()) {
              failureCount++;
            } else {
              QFileInfo fileInfo(sourcePath);
              QString baseName = fileInfo.baseName();
              QString uniqueBaseName = baseName;
              int suffix = 1;
              while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
                     QFile::exists(
                         QDir(outputDir).filePath(uniqueBaseName + "." + ext))) {
                uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
              }
              usedBaseNames.insert(uniqueBaseName.toLower());

              QString baseOutputName = QDir(outputDir).filePath(uniqueBaseName);
              if (dragWidget->saveImage(&image, baseOutputName, quality,
                                        &targetExt)) {
                successCount++;
                totalOrigBytes += fileInfo.size();
                QString outputFilePath = baseOutputName + "." + ext;
                totalOutputBytes += QFileInfo(outputFilePath).size();
              } else {
                failureCount++;
              }
            }

            QMetaObject::invokeMethod(
                this,
                [this, i]() {
                  m_progressBar->setValue(i + 1);
                },
                Qt::QueuedConnection);
          }

          QMetaObject::invokeMethod(
              this,
              [this, successCount, failureCount, totalOrigBytes,
               totalOutputBytes]() {
                setProcessingState(false);
                m_cancelButton->setText("Cancel Conversion");

                QString title = m_isCancelled ? "Batch Conversion Cancelled"
                                              : "Batch Conversion Complete";
                QString message =
                    QString("%1 image(s) converted successfully.\n%2 image(s) failed.")
                        .arg(successCount)
                        .arg(failureCount);

                if (totalOrigBytes > 0 && totalOutputBytes > 0 &&
                    successCount > 0) {
                  double diffPct =
                      (1.0 - (static_cast<double>(totalOutputBytes) /
                              totalOrigBytes)) *
                      100.0;
                  message +=
                      QString("\n\nTotal Original: %1\nTotal Converted: %2 (%3% %4)")
                          .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                          .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                          .arg(qAbs(diffPct), 0, 'f', 1)
                          .arg(diffPct >= 0 ? "smaller" : "larger");
                }

                if (m_isCancelled) {
                  message += "\n(Operation was stopped early by user)";
                }

                MessageBoxWidget messageBox(title, message,
                                            MessageBoxWidget::Information, this);
                messageBox.exec();
              },
              Qt::QueuedConnection);
        });
    m_futureWatcher->setFuture(future);
  }
}

void MainPage::onImageTargetExtensionChanged() {
  int value = m_targetExtensionSelect ? m_targetExtensionSelect->currentIndex() : 0;
  if (value == 0) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 1) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPEG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 2) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PNG;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("PNG (Lossless - Quality N/A)");
  } else if (value == 3) {
    m_targetImageExtension = DropFileWidget::ImageExtension::WEBP;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 4) {
    m_targetImageExtension = DropFileWidget::ImageExtension::TIFF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 5) {
    m_targetImageExtension = DropFileWidget::ImageExtension::BMP;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("BMP (Uncompressed)");
  } else if (value == 6) {
    m_targetImageExtension = DropFileWidget::ImageExtension::GIF;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("GIF (Indexed Color)");
  } else if (value == 7) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PDF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("PDF Image Quality");
  }
}

```

### File 6 of 10: `pages/pdf_page.h`

Overwrite `pages/pdf_page.h` with the following complete code:

```
#ifndef PDF_PAGE_H
#define PDF_PAGE_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include "widgets/message_box_widget.h"
#include "widgets/slider_widget.h"
#include <QFutureWatcher>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>
#include <functional>

class PdfPage : public QWidget {
  Q_OBJECT

public:
  explicit PdfPage(QWidget *parent = nullptr);
  ~PdfPage() override;

private slots:
  void onProcessButtonClicked();
  void onCancelButtonClicked();
  void onPresetChanged(int index);
  void onQualitySliderChanged();
  void applyThemeStyles();

private:
  QVBoxLayout *mainLayout{nullptr};
  DropFileWidget *m_dragWidget{nullptr};
  SliderWidget *m_qualitySlider{nullptr};
  ui::Select *m_presetSelect{nullptr};
  ui::FormField *m_presetField{nullptr};
  QProgressBar *m_progressBar{nullptr};
  ui::Button *m_processButton{nullptr};
  ui::Button *m_cancelButton{nullptr};
  QFutureWatcher<void> *m_futureWatcher{nullptr};
  std::atomic<bool> m_isCancelled{false};
  bool m_isProcessing{false};

  void setProcessingState(bool isProcessing);

  void compressSinglePdf(const QString &sourcePath, int quality);
  void compressBatchPdf(const QStringList &sourcePaths, int quality);
  bool compressPdf(const QString &inputPath, const QString &outputPath,
                   int quality,
                   const std::function<void(int, int)> &onPageProgress);
  bool compressPdfWithGhostscript(const QString &inputPath,
                                  const QString &tempOutputPath, int quality);
  bool compressPdfWithQtFallback(
      const QString &inputPath, const QString &tempOutputPath, int quality,
      const std::function<void(int, int)> &onPageProgress);
  static QString findGhostscriptExecutable();
  QImage compressImage(const QImage &originalImage, int quality);
  int calculateDPI(int quality);
};

#endif // PDF_PAGE_H

```

### File 7 of 10: `pages/pdf_page.cpp`

Overwrite `pages/pdf_page.cpp` with the following complete code:

```
#include "pdf_page.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

namespace {
bool replaceDestinationAtomically(const QString &tempOutputPath,
                                  const QString &outputPath) {
  const bool hadExisting = QFile::exists(outputPath);
  const QString backupPath = outputPath + ".bak";
  if (hadExisting) {
    QFile::remove(backupPath);
    if (!QFile::rename(outputPath, backupPath)) {
      QFile::remove(tempOutputPath);
      return false;
    }
  }
  if (!QFile::rename(tempOutputPath, outputPath)) {
    QFile::remove(tempOutputPath);
    if (hadExisting) {
      QFile::rename(backupPath, outputPath);
    }
    return false;
  }
  if (hadExisting) {
    QFile::remove(backupPath);
  }
  return true;
}
} // namespace

PdfPage::PdfPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(14);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  m_dragWidget = new DropFileWidget(this, "PDF");
  m_qualitySlider = new SliderWidget(this, "PDF Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_qualitySlider->setValue(50);

  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);

  QStringList presetOptions = {
      "Custom", "Web / Email (70-80% smaller)", "Balanced (40-50% smaller)",
      "High Quality (15-25% smaller)"};
  m_presetSelect = new ui::Select(presetOptions, this);
  m_presetSelect->setCurrentIndex(2); // Default to Balanced (50)
  m_presetField = new ui::FormField("Compression Preset", m_presetSelect, this);

  connect(m_presetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &PdfPage::onPresetChanged);
  connect(m_qualitySlider, &SliderWidget::valueChanged, this,
          &PdfPage::onQualitySliderChanged);

  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_presetField, 1, Qt::AlignTop);

  m_progressBar = new QProgressBar(this);
  m_progressBar->setFixedHeight(22);
  m_progressBar->setAlignment(Qt::AlignCenter);
  m_progressBar->setTextVisible(true);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Compress PDF", ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &PdfPage::onProcessButtonClicked);

  m_cancelButton = new ui::Button("Cancel Compression",
                                  ui::ButtonVariant::Destructive,
                                  ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &PdfPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  mainLayout->addLayout(attributeLayout);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &PdfPage::applyThemeStyles);
  applyThemeStyles();
}

PdfPage::~PdfPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void PdfPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  m_progressBar->setFont(t.font(t.sizeXs, QFont::DemiBold));
  m_progressBar->setStyleSheet(
      QString("QProgressBar {"
              "  border: 1px solid %1;"
              "  border-radius: %2px;"
              "  text-align: center;"
              "  background-color: %3;"
              "  color: %4;"
              "}"
              "QProgressBar::chunk {"
              "  background-color: %5;"
              "  border-radius: %6px;"
              "}")
          .arg(ui::StyleHelper::toHexString(c.border))
          .arg(r.md)
          .arg(ui::StyleHelper::toHexString(c.muted))
          .arg(ui::StyleHelper::toHexString(c.foreground))
          .arg(ui::StyleHelper::toHexString(c.primary))
          .arg(r.md > 2 ? r.md - 1 : 2));

  if (m_processButton) {
    m_processButton->setIcon(ui::Icon::get(":/icons/icons/lucide-file-text.svg",
                                           c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(
        ":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void PdfPage::onPresetChanged(int index) {
  if (!m_qualitySlider) return;
  m_qualitySlider->blockSignals(true);
  if (index == 1) {
    m_qualitySlider->setValue(20);
  } else if (index == 2) {
    m_qualitySlider->setValue(50);
  } else if (index == 3) {
    m_qualitySlider->setValue(80);
  }
  m_qualitySlider->blockSignals(false);
}

void PdfPage::onQualitySliderChanged() {
  if (!m_presetSelect || !m_qualitySlider) return;
  int val = m_qualitySlider->getValue();
  m_presetSelect->blockSignals(true);
  if (val == 20) {
    m_presetSelect->setCurrentIndex(1);
  } else if (val == 50) {
    m_presetSelect->setCurrentIndex(2);
  } else if (val == 80) {
    m_presetSelect->setCurrentIndex(3);
  } else {
    m_presetSelect->setCurrentIndex(0);
  }
  m_presetSelect->blockSignals(false);
}

void PdfPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  m_progressBar->setVisible(isProcessing);
  m_qualitySlider->setEnabled(!isProcessing);
  if (m_presetSelect) {
    m_presetSelect->setEnabled(!isProcessing);
  }
  m_dragWidget->setEnabled(!isProcessing);
}

void PdfPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Cancelling...");
  }
}

void PdfPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    MessageBoxWidget messageBox("Error", "Please select at least one PDF file.",
                                MessageBoxWidget::Warning, this);
    messageBox.exec();
    return;
  }

  int compressionQuality = m_qualitySlider->getValue();

  if (sourcePaths.size() == 1) {
    compressSinglePdf(sourcePaths.first(), compressionQuality);
  } else {
    compressBatchPdf(sourcePaths, compressionQuality);
  }
}

void PdfPage::compressSinglePdf(const QString &sourcePath, int quality) {
  QString outputPath = QFileDialog::getSaveFileName(
      this, "Save Compressed PDF",
      QFileInfo(sourcePath)
          .dir()
          .filePath(QFileInfo(sourcePath).baseName() + "_compressed.pdf"),
      "PDF Files (*.pdf)");

  if (outputPath.isEmpty()) {
    return;
  }

  setProcessingState(true);
  m_isCancelled = false;
  m_progressBar->setRange(0, 100);
  m_progressBar->setValue(0);
  m_progressBar->setFormat("Compressing PDF... %p%");

  QFuture<void> future =
      QtConcurrent::run([this, sourcePath, outputPath, quality]() {
        auto progressCallback = [this](int current, int total) {
          QMetaObject::invokeMethod(
              this,
              [this, current, total]() {
                m_progressBar->setRange(0, total);
                m_progressBar->setValue(current);
                m_progressBar->setFormat(
                    QString("Compressing page %v of %m (%p%)"));
              },
              Qt::QueuedConnection);
        };

        bool success =
            compressPdf(sourcePath, outputPath, quality, progressCallback);

        QMetaObject::invokeMethod(
            this,
            [this, sourcePath, success, outputPath]() {
              setProcessingState(false);
              m_cancelButton->setText("Cancel Compression");

              if (success) {
                qint64 origSize = QFileInfo(sourcePath).size();
                qint64 compressedSize = QFileInfo(outputPath).size();
                QString sizeStats;
                if (origSize > 0 && compressedSize > 0) {
                  double diffPct =
                      (1.0 - (static_cast<double>(compressedSize) / origSize)) *
                      100.0;
                  sizeStats =
                      QString("\n\nOriginal: %1\nCompressed: %2 (%3% %4)")
                          .arg(DropFileWidget::formatFileSize(origSize))
                          .arg(DropFileWidget::formatFileSize(compressedSize))
                          .arg(qAbs(diffPct), 0, 'f', 1)
                          .arg(diffPct >= 0 ? "smaller" : "larger");
                }

                MessageBoxWidget messageBox(
                    "Success",
                    QString("PDF compressed successfully!\nSaved to: %1%2")
                        .arg(outputPath)
                        .arg(sizeStats),
                    MessageBoxWidget::Information, this);
                messageBox.exec();
              } else if (m_isCancelled) {
                MessageBoxWidget messageBox(
                    "Cancelled", "PDF compression was cancelled.",
                    MessageBoxWidget::Information, this);
                messageBox.exec();
              } else {
                MessageBoxWidget messageBox(
                    "Error", "Failed to compress PDF. Please try again.",
                    MessageBoxWidget::Critical, this);
                messageBox.exec();
              }
            },
            Qt::QueuedConnection);
      });
  m_futureWatcher->setFuture(future);
}

void PdfPage::compressBatchPdf(const QStringList &sourcePaths, int quality) {
  QString outputDir = QFileDialog::getExistingDirectory(
      this, "Select Output Directory for Compressed PDFs", QDir::homePath());

  if (outputDir.isEmpty()) {
    return;
  }

  setProcessingState(true);
  m_isCancelled = false;
  int totalFiles = sourcePaths.size();
  m_progressBar->setRange(0, totalFiles);
  m_progressBar->setValue(0);
  m_progressBar->setFormat(QString("Compressing %v of %m PDFs (%p%)"));

  QFuture<void> future = QtConcurrent::run(
      [this, sourcePaths, outputDir, quality, totalFiles]() {
        int successCount = 0;
        int failureCount = 0;
        qint64 totalOrigBytes = 0;
        qint64 totalOutputBytes = 0;
        QSet<QString> usedBaseNames;

        for (int i = 0; i < totalFiles; ++i) {
          if (m_isCancelled) {
            break;
          }

          const QString &sourcePath = sourcePaths.at(i);
          QFileInfo fileInfo(sourcePath);
          QString baseName = fileInfo.baseName() + "_compressed";
          QString uniqueBaseName = baseName;
          int suffix = 1;
          while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
                 QFile::exists(
                     QDir(outputDir).filePath(uniqueBaseName + ".pdf"))) {
            uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
          }
          usedBaseNames.insert(uniqueBaseName.toLower());

          QString outputPath =
              QDir(outputDir).filePath(uniqueBaseName + ".pdf");

          if (compressPdf(sourcePath, outputPath, quality, nullptr)) {
            successCount++;
            totalOrigBytes += fileInfo.size();
            totalOutputBytes += QFileInfo(outputPath).size();
          } else {
            failureCount++;
          }

          QMetaObject::invokeMethod(
              this, [this, i]() { m_progressBar->setValue(i + 1); },
              Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(
            this,
            [this, successCount, failureCount, totalOrigBytes,
             totalOutputBytes]() {
              setProcessingState(false);
              m_cancelButton->setText("Cancel Compression");

              QString title = m_isCancelled ? "Batch Compression Cancelled"
                                            : "Batch Compression Complete";
              QString message =
                  QString("%1 PDF(s) compressed successfully.\n%2 PDF(s) failed.")
                      .arg(successCount)
                      .arg(failureCount);

              if (totalOrigBytes > 0 && totalOutputBytes > 0 &&
                  successCount > 0) {
                double diffPct =
                    (1.0 - (static_cast<double>(totalOutputBytes) /
                            totalOrigBytes)) *
                    100.0;
                message +=
                    QString("\n\nTotal Original: %1\nTotal Compressed: %2 (%3% %4)")
                        .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                        .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                        .arg(qAbs(diffPct), 0, 'f', 1)
                        .arg(diffPct >= 0 ? "smaller" : "larger");
              }

              if (m_isCancelled) {
                message += "\n(Operation was stopped early by user)";
              }

              MessageBoxWidget messageBox(title, message,
                                          MessageBoxWidget::Information, this);
              messageBox.exec();
            },
            Qt::QueuedConnection);
      });
  m_futureWatcher->setFuture(future);
}

QString PdfPage::findGhostscriptExecutable() {
  const QString appDir = QCoreApplication::applicationDirPath();
  const QStringList bundledCandidates = {
      QDir(appDir).filePath("gs/bin/gswin64c.exe"),
      QDir(appDir).filePath("gswin64c.exe"),
      QDir(appDir).filePath("gs/bin/gs")};
  for (const QString &candidate : bundledCandidates) {
    if (QFileInfo::exists(candidate) && QFileInfo(candidate).isExecutable()) {
      return candidate;
    }
  }

  const QStringList systemNames = {"gswin64c", "gswin32c", "gs"};
  for (const QString &name : systemNames) {
    QString found = QStandardPaths::findExecutable(name);
    if (!found.isEmpty()) {
      return found;
    }
  }
  return QString();
}

bool PdfPage::compressPdfWithGhostscript(const QString &inputPath,
                                         const QString &tempOutputPath,
                                         int quality) {
  QString gsBin = findGhostscriptExecutable();
  if (gsBin.isEmpty()) {
    return false;
  }

  int clampedQuality = qBound(1, quality, 100);
  int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
  QString pdfSetting = (clampedQuality <= 30)   ? "/screen"
                       : (clampedQuality <= 70) ? "/ebook"
                                                : "/printer";

  QStringList args = {
      "-sDEVICE=pdfwrite",
      "-dCompatibilityLevel=1.5",
      QString("-dPDFSETTINGS=%1").arg(pdfSetting),
      "-dNOPAUSE",
      "-dQUIET",
      "-dBATCH",
      "-dDetectDuplicateImages=true",
      "-dCompressFonts=true",
      "-dSubsetFonts=true",
      "-dAutoFilterColorImages=false",
      "-dAutoFilterGrayImages=false",
      "-dColorImageFilter=/DCTEncode",
      "-dGrayImageFilter=/DCTEncode",
      "-dDownsampleColorImages=true",
      "-dDownsampleGrayImages=true",
      "-dDownsampleMonoImages=true",
      "-dColorImageDownsampleType=/Bicubic",
      "-dGrayImageDownsampleType=/Bicubic",
      QString("-dColorImageResolution=%1").arg(targetDpi),
      QString("-dGrayImageResolution=%1").arg(targetDpi),
      QString("-dMonoImageResolution=%1").arg(targetDpi * 2),
      QString("-sOutputFile=%1").arg(QDir::toNativeSeparators(tempOutputPath)),
      QDir::toNativeSeparators(inputPath)};

  QProcess proc;
  proc.start(gsBin, args);
  while (!proc.waitForFinished(150)) {
    if (m_isCancelled) {
      proc.kill();
      proc.waitForFinished(1000);
      QFile::remove(tempOutputPath);
      return false;
    }
  }

  if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0 ||
      !QFileInfo::exists(tempOutputPath) ||
      QFileInfo(tempOutputPath).size() == 0) {
    QFile::remove(tempOutputPath);
    return false;
  }

  return true;
}

bool PdfPage::compressPdfWithQtFallback(
    const QString &inputPath, const QString &tempOutputPath, int quality,
    const std::function<void(int, int)> &onPageProgress) {
  QPdfDocument pdfDocument;
  QPdfDocument::Error error = pdfDocument.load(inputPath);
  if (error != QPdfDocument::Error::None) {
    qWarning() << "Failed to load PDF:" << inputPath
               << "Error:" << static_cast<int>(error);
    return false;
  }

  int totalPages = pdfDocument.pageCount();
  if (totalPages <= 0) {
    qWarning() << "PDF has no pages:" << inputPath;
    return false;
  }

  QPdfWriter pdfWriter(tempOutputPath);
  int dpi = calculateDPI(quality);
  pdfWriter.setResolution(dpi);
  pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

  QSizeF firstPageSize = pdfDocument.pagePointSize(0);
  pdfWriter.setPageSize(QPageSize(firstPageSize, QPageSize::Point));

  QPainter painter(&pdfWriter);
  if (!painter.isActive()) {
    qWarning() << "Failed to create painter for output PDF";
    QFile::remove(tempOutputPath);
    return false;
  }
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.setRenderHint(QPainter::Antialiasing);

  for (int i = 0; i < totalPages; ++i) {
    if (m_isCancelled) {
      painter.end();
      QFile::remove(tempOutputPath);
      return false;
    }

    QSizeF pageSize = pdfDocument.pagePointSize(i);
    pdfWriter.setPageSize(QPageSize(pageSize, QPageSize::Point));

    if (i > 0) {
      if (!pdfWriter.newPage()) {
        qWarning() << "Failed to create new page" << i;
        painter.end();
        QFile::remove(tempOutputPath);
        return false;
      }
    }

    // Unified DPI calculation: 1 point = 1/72 inch, so pixel size = pt * dpi / 72.0
    QSize renderSize(qMax(1, qRound(pageSize.width() * dpi / 72.0)),
                     qMax(1, qRound(pageSize.height() * dpi / 72.0)));

    QImage pageImage = pdfDocument.render(i, renderSize);
    if (pageImage.isNull()) {
      qWarning() << "Failed to render page" << i;
      painter.end();
      QFile::remove(tempOutputPath);
      return false;
    }

    pageImage = compressImage(pageImage, quality);

    QRect pageRect =
        pdfWriter.pageLayout().paintRectPixels(pdfWriter.resolution());
    painter.drawImage(pageRect, pageImage);

    if (onPageProgress) {
      onPageProgress(i + 1, totalPages);
    }
  }

  painter.end();
  return !m_isCancelled;
}

bool PdfPage::compressPdf(const QString &inputPath, const QString &outputPath,
                          int quality,
                          const std::function<void(int, int)> &onPageProgress) {
  QString tempOutputPath = outputPath + ".tmp";
  QFile::remove(tempOutputPath);

  // 1. Try vector-preserving Ghostscript engine first (like PDF24 / iLovePDF)
  bool ok = compressPdfWithGhostscript(inputPath, tempOutputPath, quality);
  if (ok && onPageProgress) {
    onPageProgress(100, 100);
  }

  // 2. Fallback to corrected Qt PDF engine if Ghostscript is not available
  if (!ok && !m_isCancelled) {
    ok = compressPdfWithQtFallback(inputPath, tempOutputPath, quality,
                                   onPageProgress);
  }

  if (!ok || m_isCancelled) {
    QFile::remove(tempOutputPath);
    return false;
  }

  // 3. Size inflation guard: never output a file larger than the original PDF
  qint64 origBytes = QFileInfo(inputPath).size();
  qint64 newBytes = QFileInfo(tempOutputPath).size();
  if (origBytes > 0 && newBytes >= origBytes) {
    QFile::remove(tempOutputPath);
    if (!QFile::copy(inputPath, tempOutputPath)) {
      return false;
    }
  }

  return replaceDestinationAtomically(tempOutputPath, outputPath);
}

QImage PdfPage::compressImage(const QImage &originalImage, int quality) {
  if (originalImage.isNull()) {
    return originalImage;
  }

  // Paint onto opaque white canvas and convert to RGB888 (drops alpha overhead without noisy dithering)
  QImage rgbImage(originalImage.size(), QImage::Format_RGB888);
  rgbImage.fill(Qt::white);
  {
    QPainter p(&rgbImage);
    p.drawImage(0, 0, originalImage);
  }

  int jpegQuality = qBound(25, quality, 95);
  QByteArray jpegData;
  QBuffer buffer(&jpegData);
  if (buffer.open(QIODevice::WriteOnly)) {
    if (rgbImage.save(&buffer, "JPG", jpegQuality)) {
      buffer.close();
      QImage decoded;
      if (decoded.loadFromData(jpegData, "JPG")) {
        return decoded;
      }
    }
  }

  return rgbImage;
}

int PdfPage::calculateDPI(int quality) {
  int minDPI = 72;
  int maxDPI = 200;
  return minDPI + ((maxDPI - minDPI) * qBound(0, quality, 100)) / 100;
}

```

### File 8 of 10: `CMakeLists.txt`

Overwrite `CMakeLists.txt` with the following complete code:

```
cmake_minimum_required(VERSION 3.16)

project(ImageConverter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Pdf Concurrent Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Pdf Concurrent Svg)

add_subdirectory(ui-kit)

set(PROJECT_WIDGETS
        widgets/button_action.cpp
        widgets/button_action.h
        widgets/button_toggle.cpp
        widgets/button_toggle.h
        widgets/input_dropdown.cpp
        widgets/input_dropdown.h
        widgets/input_field.cpp
        widgets/input_field.h
        widgets/input_upload.cpp
        widgets/input_upload.h
        widgets/input_invalid.cpp
        widgets/input_invalid.h
        widgets/input_widget.cpp
        widgets/input_widget.h
        widgets/sidebar_panel.cpp
        widgets/sidebar_panel.h
        widgets/button_sidebar_active.cpp
        widgets/button_sidebar_active.h
        widgets/button_sidebar_widget.cpp
        widgets/button_sidebar_widget.h
        widgets/drop_file_widget.h
        widgets/drop_file_widget.cpp
        widgets/slider_widget.h
        widgets/slider_widget.cpp
        widgets/message_box_widget.cpp
        widgets/message_box_widget.h
)

set(PROJECT_STYLES
        styles/colors.h
        styles/text_style.h
        resources/input_type.h
        resources/icon_type.h
        resources/icons.qrc
)

set(PROJECT_PAGES
        pages/main_page.cpp
        pages/main_page.h
        pages/pdf_page.cpp
        pages/pdf_page.h
        pages/gallery_page.cpp
        pages/gallery_page.h
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow/mainwindow.cpp
        mainwindow/mainwindow.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ImageConverter
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${PROJECT_WIDGETS}
        ${PROJECT_STYLES}
        ${PROJECT_PAGES}
    )
else()
    if(ANDROID)
        add_library(ImageConverter SHARED
            ${PROJECT_SOURCES}
            ${PROJECT_WIDGETS}
            ${PROJECT_STYLES}
            ${PROJECT_PAGES}
        )
    else()
        add_executable(ImageConverter
            ${PROJECT_SOURCES}
            ${PROJECT_WIDGETS}
            ${PROJECT_STYLES}
            ${PROJECT_PAGES}
        )
    endif()
endif()

target_link_libraries(ImageConverter PRIVATE ui_kit Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Pdf Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Svg)
target_include_directories(ImageConverter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ImageConverter)
endif()
set_target_properties(ImageConverter PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ImageConverter
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES resources/image-converter.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES resources/icons/icon-logo.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps
        RENAME image-converter.png)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ImageConverter)
endif()

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "image-converter")
set(CPACK_PACKAGE_VENDOR "Illufox Kusanagi")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast offline image converter and PDF compressor built with Qt 6")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Debian (.deb)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Illufox Kusanagi")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# RPM (.rpm)
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

include(CPack)

```

### File 9 of 10: `ui-kit/components/table.h`

Overwrite `ui-kit/components/table.h` (previously 0 bytes) with the following complete code:

```
#ifndef UI_TABLE_H
#define UI_TABLE_H

#include "../theme/theme.h"
#include <QTableWidget>

namespace ui {

class Table : public QTableWidget {
  Q_OBJECT

public:
  explicit Table(QWidget *parent = nullptr);
  Table(int rows, int columns, QWidget *parent = nullptr);
  ~Table() override = default;

private slots:
  void applyThemeStyles();
};

} // namespace ui

#endif // UI_TABLE_H

```

### File 10 of 10: `ui-kit/components/table.cpp`

Overwrite `ui-kit/components/table.cpp` (previously 0 bytes) with the following complete code:

```
#include "table.h"
#include "../theme/style_helper.h"
#include <QHeaderView>

namespace ui {

Table::Table(QWidget *parent) : Table(0, 0, parent) {}

Table::Table(int rows, int columns, QWidget *parent)
    : QTableWidget(rows, columns, parent) {
  setAlternatingRowColors(false);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setShowGrid(false);
  verticalHeader()->setVisible(false);
  horizontalHeader()->setStretchLastSection(true);
  horizontalHeader()->setHighlightSections(false);

  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Table::applyThemeStyles);
  applyThemeStyles();
}

void Table::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  setFont(t.font(t.sizeSm, QFont::Normal));
  horizontalHeader()->setFont(t.font(t.sizeXs, QFont::DemiBold));

  setStyleSheet(
      QString("QTableWidget {"
              "  background-color: %1;"
              "  color: %2;"
              "  border: 1px solid %3;"
              "  border-radius: %4px;"
              "  gridline-color: %3;"
              "  outline: none;"
              "}"
              "QTableWidget::item {"
              "  padding: 6px 10px;"
              "  border-bottom: 1px solid %3;"
              "}"
              "QTableWidget::item:selected {"
              "  background-color: %5;"
              "  color: %6;"
              "}"
              "QHeaderView::section {"
              "  background-color: %7;"
              "  color: %8;"
              "  padding: 6px 10px;"
              "  border: none;"
              "  border-bottom: 1px solid %3;"
              "}")
          .arg(StyleHelper::toHexString(c.card))
          .arg(StyleHelper::toHexString(c.foreground))
          .arg(StyleHelper::toHexString(c.border))
          .arg(r.md)
          .arg(StyleHelper::toHexString(c.accent))
          .arg(StyleHelper::toHexString(c.accentForeground))
          .arg(StyleHelper::toHexString(c.muted))
          .arg(StyleHelper::toHexString(c.mutedForeground)));
}

} // namespace ui

```

## 4. Verification Commands for the AI Agent

After overwriting all 10 files, the AI agent must run the following PowerShell verification commands and report the raw output:

```
# 1. Verify all 10 modified files have Length > 0
Get-Item `
  "main.cpp", `
  "mainwindow/mainwindow.h", `
  "mainwindow/mainwindow.cpp", `
  "widgets/drop_file_widget.cpp", `
  "pages/main_page.cpp", `
  "pages/pdf_page.h", `
  "pages/pdf_page.cpp", `
  "CMakeLists.txt", `
  "ui-kit/components/table.h", `
  "ui-kit/components/table.cpp" | Select-Object FullName, Length

# 2. Configure & Build
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j4

```