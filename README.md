# ImageConverter

[![License: MIT](https://img.shields.io/badge/License-MIT-red.svg)](https://opensource.org/licenses/MIT)
[![Release](https://img.shields.io/github/v/release/illufoxKusanagi/Image-Converter?display_name=tag&sort=semver)](https://github.com/illufoxKusanagi/image-converter/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)](#)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6.x-green)](https://www.qt.io/)

<img width="64" alt="icon-logo" src="https://github.com/user-attachments/assets/8e5b16d0-39c2-4b11-a517-651b18d27001" />

A fast, native, **100% offline** C++17 / Qt 6 desktop application for batch image conversion and PDF compression, built with a custom Shadcn-inspired Qt Widgets UI-Kit.

---

## 📋 Overview

**ImageConverter** is a lightweight, privacy-first desktop utility designed to convert single or batch images across common raster and document formats and compress multi-page PDF documents locally. Every operation runs locally on your machine with **zero internet requirement**, **zero telemetry**, and **completely free** open-source licensing.

---

## ✨ Key Features

### 🖼️ Image Conversion (`MainPage`)
- **Multi-Format Support**: Convert single or batch images across `JPG`, `JPEG`, `PNG`, `WEBP`, `TIFF`, `BMP`, `GIF`, and `PDF`.
- **Drag-and-Drop Queue (`DropFileWidget`)**: Drop multiple files at once with automatic duplicate detection and individual queue item removal.
- **Precision Quality Control**: Synchronized quality slider and spinbox (`0–100`) with automatic lossless (`-1`) handling for `PNG`.
- **Multi-Threaded Batch Pipeline**: Asynchronous background processing powered by `QtConcurrent` and `QFutureWatcher` with live progress reporting and instant cancellation support.

### 📄 PDF Compression (`PdfPage`)
- **Local Batch PDF Compression**: Compress single or multiple multi-page PDF documents directly on your machine.
- **Compression Presets**: Choose between `Extreme`, `Recommended`, `High Quality`, and `Custom` presets synchronized with the quality slider.
- **Dual-Engine Pipeline**:
  - **Ghostscript Engine (`gs` / `gswin64c`)**: Automatically used when installed on the system to preserve crisp vector text and embedded fonts while downsampling images (`72–220 DPI`).
  - **Native Qt Fallback (`QPdfDocument` + `QPdfWriter`)**: Seamless built-in page rasterization, resolution scaling, and color quantization when Ghostscript is not present.
- **Smart Size Guard**: Automatically retains the original PDF whenever re-encoding would not reduce the file size.

### 🎨 Shadcn-Inspired Qt UI-Kit (`ui-kit/`) & Component Gallery (`GalleryPage`)
- **Reactive Theme Engine (`ui::Theme`)**: Instant switching between **Light**, **Dark**, and **System** (OS-synchronized) themes without restarting.
- **Tokenized Design System**: Centralized `ColorTokens` (including semantic `success`, `warning`, `info`, and `destructive` tokens), `RadiusTokens`, `SpacingTokens`, and `TypographyTokens` with `rem` scaling.
- **30 Reusable C++ Qt Widgets Components**:
  - `Accordion`, `Alert`, `Avatar`, `Badge`, `Button` (with child `Spinner` loading state), `Card`, `Checkbox` (including tri-state & error state), `Combobox` (searchable model/proxy popup), `Dialog`, `EmptyState`, `FormField`, `Input`, `Menu` (`DropdownMenu` / `ContextMenu`), `NumberInput` / `DecimalInput`, `Popover`, `Progress`, `RadioGroup`, `ScrollArea`, `Select`, `Separator`, `Sidebar`, `Slider`, `Spinner`, `Splitter` (keyboard-accessible handle), `Switch`, `Table`, `Tabs`, `Textarea`, `Toast`, and `Tooltip`.
- **Interactive UI Components Showcase (`GalleryPage`)**: Built-in **UI Components** tab demonstrating component variants, sizes, states, and live dialogs/toasts.

### 🔒 100% Offline & Free
- Zero network calls, zero analytics or telemetry, and no external web dependencies.

---

## 🖼️ Screenshots

![image](https://github.com/user-attachments/assets/cb1fe34c-7bcc-4edc-bfa9-b652a2932f5d)

---

## 🛠️ Tech Stack

| Layer | Technology |
| :--- | :--- |
| **Language** | C++17 (`CMAKE_CXX_STANDARD 17`) |
| **Framework** | Qt 6 (`Core`, `Widgets`, `Pdf`, `Concurrent`, `Svg`) with Qt 5 fallback support |
| **UI Architecture** | Custom static `ui_kit` library (`ui-kit/`) + Lucide SVG icons via `CMAKE_AUTORCC` |
| **Build & Packaging** | CMake 3.16+, Ninja, CPack (`.deb`, `.rpm`, Windows portable `.zip`) |
| **Optional Runtime Tool** | Ghostscript (`ghostscript`) for vector-preserving PDF compression |

---

## 📦 Installation (Pre-built Releases)

Download the latest pre-built packages from [**GitHub Releases**](https://github.com/illufoxKusanagi/image-converter/releases):

### Windows (Portable `.zip`)
1. Download `ImageConverter-windows-x64.zip` from the latest release.
2. Extract the archive to any folder.
3. Run `ImageConverter.exe` (no installer required).

### Debian / Ubuntu (`.deb`)
```bash
sudo apt install ./image-converter-*.deb
```

### Fedora / RHEL (`.rpm`)
```bash
sudo dnf install ./image-converter-*.rpm
```

---

## 🚀 Usage

### 1. Convert Images
1. Open the **Convert Image** tab.
2. **Drag and drop** image files into the drop zone or click to browse.
3. Adjust the **Image Quality** slider (`1–100`) and select your target **Output Format** (`JPG`, `JPEG`, `PNG`, `WEBP`, `TIFF`, `BMP`, `GIF`, or `PDF`).
4. Click **Convert Files** and choose the destination file or output directory.

### 2. Compress PDF
1. Switch to the **Compress PDF** tab.
2. Add one or more `.pdf` files via drag-and-drop or the file browser.
3. Select a compression preset (**Extreme**, **Recommended**, **High Quality**, or **Custom**) or fine-tune the quality slider.
4. Click **Compress PDF** and select where to save the optimized PDF(s).

### 3. Theme & UI Component Gallery
- Click the **Theme** button in the top-right header bar to cycle between **System**, **Light**, and **Dark** themes.
- Switch to the **UI Components** tab to explore and interactively test all 30 components in the custom `ui_kit` library.

---

## 📁 Project Structure

```text
ImageConverter/
├── .github/workflows/        # CI/CD release pipelines (Linux .deb/.rpm & Windows .zip)
├── ui-kit/                   # Shadcn-inspired Qt Widgets component library (static lib: ui_kit)
│   ├── theme/                # Tokens, Theme manager, StyleHelper, AnimationHelper, Icons
│   └── components/           # 30 reusable UI components (Button, Card, Combobox, Dialog, etc.)
├── mainwindow/               # Application shell window, header bar, theme toggle & tab routing
│   ├── mainwindow.h
│   └── mainwindow.cpp
├── pages/                    # Primary application views
│   ├── main_page.h/.cpp      # Image conversion workflow & asynchronous batch runner
│   ├── pdf_page.h/.cpp       # PDF compression workflow (Ghostscript + QPdfDocument fallback)
│   └── gallery_page.h/.cpp   # Interactive showcase for all ui-kit components
├── widgets/                  # Application-specific composite widgets
│   ├── drop_file_widget.*    # Drag-and-drop file queue & image conversion helpers
│   ├── slider_widget.*       # Synchronized slider + spinbox quality control
│   ├── button_action.*       # Action button wrapper
│   ├── input_widget.*        # Labeled form input & format selector
│   └── message_box_widget.*  # Modal alert dialog wrapper
├── styles/                   # Legacy text and color helpers
├── resources/                # Qt Resource manifest (icons.qrc) & Lucide SVG icons
├── CMakeLists.txt            # Root CMake build & CPack packaging configuration
└── main.cpp                  # Application entry point & theme initialization
```

---

## 🔧 Building from Source

### 1. Install Dependencies

#### Ubuntu / Debian
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build \
  qt6-base-dev qt6-pdf-dev qt6-svg-dev libgl1-mesa-dev ghostscript
```

#### Fedora
```bash
sudo dnf install -y gcc-c++ cmake ninja-build \
  qt6-qtbase-devel qt6-qtpdf-devel qt6-qtsvg-devel mesa-libGL-devel ghostscript
```

#### Windows
- Install **Qt 6.x** via the [Qt Online Installer](https://www.qt.io/download) (include **Qt PDF** and **Qt 5 Compatibility / SVG** modules alongside **MSVC 2022 64-bit** or **MinGW 64-bit**).
- Ensure **CMake 3.16+** and **Ninja** are installed and available in your `PATH`.
- *(Optional)* Install **Ghostscript** (`gswin64c`) for vector-preserving PDF compression.

### 2. Compile with CMake & Ninja

```bash
git clone https://github.com/illufoxKusanagi/image-converter.git
cd image-converter

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 3. Generate Installable Packages (Optional)

```bash
cd build
cpack -G DEB   # Debian / Ubuntu (.deb)
cpack -G RPM   # Fedora / RHEL (.rpm)
cpack -G ZIP   # Portable archive (.zip)
```

### 4. Building in Qt Creator
1. Launch **Qt Creator** and open `CMakeLists.txt` from the repository root.
2. Select a configured **Qt 6** kit (MinGW 64-bit, MSVC 64-bit, or GCC/Clang on Linux).
3. Click **Build** (`Ctrl+B`) and **Run** (`Ctrl+R`).

---

## 🤝 Contributing

Contributions are welcome! To contribute:

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/new-feature`).
3. Commit your changes (`git commit -m "Add new feature"`).
4. Push to the branch (`git push origin feature/new-feature`).
5. Open a Pull Request.

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 📧 Contact

Project Repository: [https://github.com/illufoxKusanagi/image-converter](https://github.com/illufoxKusanagi/image-converter)
