---
name: image-converter-dev
description: >-
  Use this skill when developing, building, testing, extending image/PDF conversion features,
  modifying UI widgets, or debugging the ImageConverter Qt application.
---

# ImageConverter Developer Skill & Procedures

This skill provides verified workflows and step-by-step instructions for working with the **ImageConverter** Qt 6 / Qt 5 application.

---

## 1. Prerequisites & Environment Setup

To compile and build ImageConverter on Windows:
- **CMake**: Version 3.16 or higher (recommended: 3.30+)
- **Generator**: Ninja or MinGW Makefiles
- **Compiler**: LLVM-MinGW (Clang/GCC) 17.0.6 64-bit or GCC MinGW-w64 64-bit
- **Qt SDK**: Qt 6.x (recommended Qt 6.5+ or 6.7+) with `Core`, `Widgets`, `Pdf` components installed.

---

## 2. Building & Running the Application

### 2.1 Standard CMake Build Commands

```powershell
# Create and navigate to build directory
mkdir build
cd build

# Configure CMake with Qt6 path (adjust QT_DIR to match local installation if needed)
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..

# Compile the target executable
cmake --build . --config Release

# Run the executable
./ImageConverter.exe
```

### 2.2 Rebuilding Resource Files
If icons or assets in `resources/icons.qrc` or `resources/icons/` are modified, CMake will automatically trigger `RCC` compilation via `set(CMAKE_AUTORCC ON)`.

---

## 3. Workflow: Adding a New Image Format

Follow these exact steps when adding a new image format (e.g. `BMP`, `AVIF`, `GIF`):

1. **Update Enum in `DropFileWidget`**:
   - In [widgets/drop_file_widget.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.h), add the new format to `enum ImageExtension { JPG, JPEG, PNG, WEBP, TIFF, BMP, ... };`.
2. **Update Dropzone Filter & Drop Handling**:
   - In [widgets/drop_file_widget.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.cpp):
     - Update `dropEvent` extension check to include `"bmp"`.
     - Update `onBrowseButtonPressed` file filter dialog string.
     - Update `imageExtensionToString` to map the new enum value.
     - In `saveImage`, add a switch case for the new format:
       ```cpp
       case BMP:
         formatString = "BMP";
         isSuccess = image->save(outputPath + ".bmp", "BMP", -1);
         break;
       ```
3. **Update Format Dropdown in `MainPage`**:
   - In [pages/main_page.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/main_page.cpp):
     - In `setupExtensionButton()`, add `"bmp"` to `QStringList extensionOptions`.
     - In `onImageTargetExtensionChanged()`, map the corresponding index to `DropFileWidget::ImageExtension::BMP`.

---

## 4. Workflow: Adding a New Application Page

To introduce a new processing tab (e.g., "Batch Watermark", "Image Resizer"):

1. **Create Page Classes**:
   - Create `pages/new_feature_page.h` and `pages/new_feature_page.cpp`.
   - Inherit from `QWidget`.
   - Implement constructor taking `QWidget *parent = nullptr`.
2. **Register in `CMakeLists.txt`**:
   - Add `pages/new_feature_page.cpp` and `pages/new_feature_page.h` to the `PROJECT_PAGES` list in [CMakeLists.txt](file:///d:/matkul/sem_6/AppProject/ImageConverter/CMakeLists.txt).
3. **Integrate into `MainWindow`**:
   - In [mainwindow/mainwindow.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow.h), include `pages/new_feature_page.h`.
   - In [mainwindow/mainwindow.cpp](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow.cpp), instantiate the page and add to `m_tabWidget`:
     ```cpp
     NewFeaturePage *featurePage = new NewFeaturePage(this);
     m_tabWidget->addTab(featurePage, "New Feature");
     ```

---

## 5. Troubleshooting & Diagnostics

| Symptom | Cause | Solution |
| :--- | :--- | :--- |
| `Qt6::Pdf not found` | Qt Pdf module not selected during Qt SDK install | Rerun Qt Maintenance Tool and check `Qt Pdf` under Qt 6.x |
| `image.isNull() returns true` | Corrupted image file or unsupported sub-codec | Ensure image header is intact; check Qt image plugins (`imageformats/`) |
| PDF compression produces blank pages | DPI/scaling calculation exceeds painter pixel bounds | Verify `calculateDPI` and `calculateScaleFactor` stay within valid ranges |
| Infinite loop / freeze on slider change | Bidirectional signal cycle between slider and spinbox | Verify `blockSignals(true)` / `blockSignals(false)` is used in `SliderWidget` |
