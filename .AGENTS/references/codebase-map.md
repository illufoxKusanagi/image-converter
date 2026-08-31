# Comprehensive Codebase Map & Symbol Catalog

## 1. File & Component Registry

### Top-Level & Entry Point
- [main.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/main.cpp): Application bootstrapping (`QApplication`), sets application/organization metadata, window icon pyramid (16x16 up to 128x128), and executes the Qt event loop.
- [CMakeLists.txt](file:///d:/matkul/sem_6/AppProject/ImageConverter/CMakeLists.txt): CMake build rules, target dependencies (`Qt6::Core`, `Qt6::Widgets`, `Qt6::Pdf`), install targets, and target properties.

---

### MainWindow Shell
- [mainwindow/mainwindow.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow.h) / [mainwindow/mainwindow.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow.cpp):
  - **Class**: `MainWindow` (inherits `QMainWindow`)
  - **Members**: `QTabWidget *m_tabWidget`
  - **Methods**: `void setupTabStyle()`
  - **Tabs**:
    - Index 0: `MainPage` ("Convert Image")
    - Index 1: `PdfPage` ("Compress PDF")
- [mainwindow/mainwindow_dummy.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow_dummy.h) / [mainwindow/mainwindow_dummy.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow_dummy.cpp): Legacy single-window prototype used during early exploration.

---

### Application Pages
- [pages/main_page.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/main_page.h) / [pages/main_page.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/main_page.cpp):
  - **Class**: `MainPage` (inherits `QWidget`)
  - **Members**:
    - `SliderWidget *m_qualitySlider`: Image quality adjustment (0–100)
    - `DropFileWidget *m_dragWidget`: Image drop target zone
    - `InputWidget *m_targetExtension`: Format selection dropdown wrapper
    - `DropFileWidget::ImageExtension m_sourceExtension`: Active target format enum
  - **Slots**:
    - `void onProcessButtonClicked()`: Handles single/batch conversion pipeline.
    - `void onImageTargetExtensionChanged()`: Maps dropdown index (0=JPG, 1=JPEG, 2=PNG, 3=WEBP, 4=TIFF) to `m_sourceExtension`.
  - **Internal Helpers**:
    - `void setupImageLayout()`
    - `void setupImageAttribute()`
    - `void setupExtensionButton()`

- [pages/pdf_page.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/pdf_page.h) / [pages/pdf_page.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/pdf_page.cpp):
  - **Class**: `PdfPage` (inherits `QWidget`)
  - **Members**:
    - `SliderWidget *m_qualitySlider`: Compression quality slider (0–100)
    - `DropFileWidget *m_dragWidget`: PDF drop target zone
  - **Slots**:
    - `void onProcessButtonClicked()`: Dispatches to single/batch compression.
  - **Engine Methods**:
    - `void compressSinglePdf(const QString &sourcePath, int quality)`
    - `void compressBatchPdf(const QStringList &sourcePaths, int quality)`
    - `bool compressPdf(const QString &inputPath, const QString &outputPath, int quality)`
    - `QImage compressImage(const QImage &originalImage, int quality)`
    - `int calculateDPI(int quality)`: Map quality to 72–200 DPI.
    - `qreal calculateScaleFactor(int quality)`: Map quality to 0.5–2.0 scale.
    - `double calculateRenderDPI(int quality)`: Compatibility helper.

---

### Custom Widgets Catalog
- [widgets/drop_file_widget.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.h) / [widgets/drop_file_widget.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.cpp):
  - **Class**: `DropFileWidget` (inherits `QGroupBox`)
  - **Enums**: `ImageExtension { JPG, JPEG, PNG, WEBP, TIFF, PDF }`
  - **Key Methods**:
    - `QString getFilePath()` / `QStringList getFilePaths()`
    - `QString imageExtensionToString(const ImageExtension &sourceExtension) const`
    - `bool saveImage(const QImage *image, const QString &outputPath, const int quality, const ImageExtension *sourceExtension)`
    - `void convertImage(const QString sourcePath)`
    - `QPixmap createColoredIcon(const QString &iconPath, const QColor &color, int width, int height)`
  - **Event Overrides**: `dragEnterEvent()`, `dropEvent()`

- [widgets/slider_widget.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/slider_widget.h) / [widgets/slider_widget.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/slider_widget.cpp):
  - **Class**: `SliderWidget` (inherits `QWidget`)
  - **Signals**: `void valueChanged()`
  - **Slots**: `void updateSliderValue()`, `void updateSpinBoxValue()`
  - **Accessor**: `int getValue()`

- [widgets/button_action.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/button_action.h) / [widgets/button_action.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/button_action.cpp):
  - **Class**: `ButtonAction` (inherits `QPushButton`)
  - **Features**: Custom primary CTA button with hover/pressed states, optional leading SVG icons.

- [widgets/input_widget.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/input_widget.h) / [widgets/input_widget.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/input_widget.cpp):
  - **Class**: `InputWidget` (inherits `QWidget`)
  - **Factory Logic**: Dynamically switches child sub-widget (`InputField`, `InputDropdown`, `InputUpload`, or `InputInvalid`) based on `InputType.type`.

- [widgets/input_dropdown.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/input_dropdown.h) / [widgets/input_dropdown.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/input_dropdown.cpp):
  - **Class**: `InputDropdown` (inherits `QWidget`)
  - **Signals**: `void valueChanged()`
  - **Methods**: `double getValue()`, `QString currentText() const`, `void setCurrentText(const QString &text)`

- [widgets/message_box_widget.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/message_box_widget.h) / [widgets/message_box_widget.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/message_box_widget.cpp):
  - **Class**: `MessageBoxWidget` (inherits `QMessageBox`)
  - **Enums**: `IconType { NoIcon, Question, Information, Warning, Critical }`

- [widgets/sidebar_panel.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/sidebar_panel.h) / [widgets/sidebar_panel.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/sidebar_panel.cpp):
  - **Class**: `SidebarPanel` (inherits `QWidget`)
  - **Components**: `ButtonToggle`, `ButtonSidebarActive` list for collapsible side navigation.

- [widgets/button_sidebar_active.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/button_sidebar_active.h) & [widgets/button_sidebar_widget.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/button_sidebar_widget.h):
  - Active & hover state handlers for sidebar items.

- [widgets/button_toggle.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/button_toggle.h) / [widgets/button_toggle.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/button_toggle.cpp):
  - Hamburger / arrow toggle button with collapse state tracking.

---

### Design System & Resources
- [styles/colors.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/colors.h): `namespace Colors` with comprehensive color definitions.
- [styles/text_style.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/text_style.h): `namespace TextStyle` typography helpers.
- [resources/icon_type.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/resources/icon_type.h): Resource path lookup helper `IconPaths::getIconPath()`.
- [resources/input_type.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/resources/input_type.h): Data structure defining input type, label, units, and read-only state.
- [resources/icons.qrc](file:///d:/matkul/sem_6/AppProject/ImageConverter/resources/icons.qrc): Qt resource configuration file.
