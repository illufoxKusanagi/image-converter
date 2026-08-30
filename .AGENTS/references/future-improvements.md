# ImageConverter — Improvement Roadmap & Audit Log (v2.2)

> Grounded in full codebase audit and reviewed against `qt-cpp-review` & `qt-ui-design` standards.

---

## 📊 Priority Matrix & Status

| Phase | Milestone | Focus Areas | Complexity | Status |
| :--- | :--- | :--- | :--- | :---: |
| **Phase 0** | **Bug Fixes & Code Cleanup** | File dialog filters, hardcoded colors, dead code, uninitialized variables | Small | 🟢 **DONE** |
| **Phase 1** | **Async Execution & Concurrency** | `QtConcurrent` workers, progress bar, cancel flag, const/API polish | Medium | 🟢 **DONE** |
| **Phase 2** | **Format-Aware Quality & Formats** | PNG lossless handling, BMP/GIF support, compression stats toast | Small | 🟡 **Next** |
| **Phase 3** | **File List & UI Scalability** | Scrollable file list, drag hover states, High-DPI & WCAG contrast fix | Medium | ⚪ Planned |
| **Phase 4** | **PDF ↔ Image Suite** | Image-to-PDF merging, PDF-to-Image extraction, compression presets | Medium | ⚪ Planned |

---

## 🔍 Audit & Review Findings Tracking

| Issue ID | Domain | Finding / Warning | Status |
| :--- | :--- | :--- | :---: |
| **[D-001]** | `qt-cpp-review` | Synchronous batch and PDF processing on main GUI event loop | 🟢 **Resolved in 1.1** |
| **[D-002]** | `qt-cpp-review` | Non-idiomatic getter prefixes (`getValue()`, `getFilePath()`, `getText()`) | 🟢 **Resolved in 1.4** |
| **[D-003]** | `qt-cpp-review` | Missing `const` qualifiers on read-only accessor methods | 🟢 **Resolved in 1.4** |
| **[D-004]** | `qt-cpp-review` | Pass-by-value `QString` in `convertImage()` | 🟢 **Resolved in 1.4** |
| **[U-001]** | `qt-ui-design` | Doherty threshold: >400ms operations lack visual progress indicator | 🟢 **Resolved in 1.2** |
| **[U-002]** | `qt-ui-design` | WCAG 2.2 AA Contrast: `Grey400` on white is ~3.2:1 (<4.5:1 required) | ⚪ Scheduled for Phase 3.4 |
| **[U-003]** | `qt-ui-design` | Fixed 352px width causes long filenames to wrap awkwardly under High-DPI | ⚪ Scheduled for Phase 3.3 |

---

## Phase 0: Bug Fixes & Code Cleanup ✅ COMPLETED

- [x] **0.1 Fixed Swapped File Dialog Filters**: PDF mode shows `"PDF Files (*.pdf)"`, Images shows `"Image Files (*.png *.jpg ...)"` ([`widgets/drop_file_widget.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.cpp)).
- [x] **0.2 Dynamic Dropzone Format Label**: PDF page shows `"pdf"`; Image page shows `"jpg, jpeg, png, webp, tiff"`.
- [x] **0.3 Silent Dialog Cancel**: Cancelling file dialogs returns cleanly without showing error popups.
- [x] **0.4 Design System Compliance**: Replaced all hardcoded hex strings (`#d1d5db`, `#ffffff`, `#f5f6fa`) in [`mainwindow/mainwindow.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/mainwindow/mainwindow.cpp), [`main.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/main.cpp), [`widgets/message_box_widget.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/message_box_widget.cpp) with `Colors::*` tokens.
- [x] **0.5 Semantic Naming**: Renamed `m_sourceExtension` to `m_targetImageExtension` across [`pages/main_page.h`](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/main_page.h) and [`pages/main_page.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/main_page.cpp).
- [x] **0.6 Fixed PDF Multi-Page Sizing**: Re-applied `pdfWriter.setPageSize()` per page in [`pages/pdf_page.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/pdf_page.cpp).
- [x] **0.7 Corrected Typos & Resources**: Fixed `Subttile*` -> `Subtitle*` in [`styles/text_style.h`](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/text_style.h); registered missing category icons in [`resources/icons.qrc`](file:///d:/matkul/sem_6/AppProject/ImageConverter/resources/icons.qrc).
- [x] **0.8 Fixed Uninitialized Variables & Static Bug**: Initialized `value` in `InputType`, `m_inputValue(0.0)` in `InputWidget`, removed static variable state corruption in `ButtonSidebarWidget`.
- [x] **0.9 Removed Dead Code & Stubs**: Removed empty stubs `setupImageInput()`, `setupQualitySlider()`, `calculateRenderDPI()`, unreferenced `mainwindow_dummy.*`, and empty `mainwindow.ui`.

---

## Phase 1: Async Concurrency & Progress ✅ COMPLETED

- [x] **1.1 Non-Blocking Background Workers (`QtConcurrent`)**:
  - Image conversion (single & batch) runs asynchronously in background thread pool.
  - Multi-page PDF compression rasterization runs in background thread pool.
  - Main GUI loop remains responsive with 0 window freeze.
- [x] **1.2 Interactive Progress Bar (`QProgressBar`)**:
  - Embedded styled progress bar in both `MainPage` and `PdfPage`.
  - Batch image conversions update per-file (`%v of %m images (%p%)`).
  - PDF compression updates per-page (`page %v of %m (%p%)`).
  - Automatically hides when idle and shows during processing.
- [x] **1.3 Cancellation Support**:
  - Replaced process button with an active **Cancel** button during processing.
  - `std::atomic<bool> m_isCancelled` gracefully aborts background loops and cleans up resources without corrupted partial writes.
- [x] **1.4 C++ API & Const-Correctness Polish**:
  - Added const qualifiers and idiomatic property accessors across `SliderWidget`, `InputWidget`, `InputDropdown`, `ButtonAction`, and `DropFileWidget`.
  - Changed `convertImage(const QString &sourcePath)` to pass by const reference.

---

## Phase 2: Format-Aware Quality & Extended Formats (Next)

### 2.1 — Smart Quality Slider Behavior
* **Problem**: The quality slider shows for PNG even though PNG uses lossless zlib compression (`-1`).
* **Solution**:
  - When target format is `PNG`: Disable lossy quality slider or switch label to "Compression Level (0–9)".
  - When target format is `JPG`/`JPEG`/`WEBP`/`TIFF`: Enable standard 0–100 quality slider.

### 2.2 — Add BMP and GIF Formats
* Add `BMP` and `GIF` to `enum ImageExtension` in [`widgets/drop_file_widget.h`](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.h).
* Add switch branches in `saveImage()` and `imageExtensionToString()`.
* Add `"bmp"`, `"gif"` options to dropdown and file filters.

### 2.3 — Compression Statistics Summary
* After conversion completes, display file size comparison in the result modal:
  ```
  Image converted successfully to photo.webp
  Original: 4.2 MB → Converted: 1.1 MB (73.8% smaller)
  ```

---

## Phase 3: File List & UI Scalability

### 3.1 — Scrollable File List View
* Replace the single `QLabel` in `DropFileWidget` with a `QScrollArea` containing individual file item cards:
  - `[icon] filename.ext  |  2.4 MB  |  [✖ remove]`
  - "Clear All" action link.

### 3.2 — Drag Hover Feedback
* Override `dragEnterEvent()` and `dragLeaveEvent()` in `DropFileWidget` to highlight borders on drag hover:
  - **Idle**: `2px solid Colors::Secondary400`
  - **Drag hover**: `3px solid Colors::Primary500` + subtle background tint

### 3.3 — Flexible Window Width & High-DPI Support
* **Problem** ([U-003]): 352px fixed width is cramped for long file paths and high-DPI scaling.
* **Solution**:
  - Replace `setFixedSize(320, 320)` with `setMinimumSize(320, 200)` and `QSizePolicy::Expanding`.
  - Widen tab widget to ~480px to give comfortable margin for file items.

### 3.4 — WCAG 2.2 Color Contrast Polish
* **Problem** ([U-002]): `Colors::Grey400` text on white background yields ~3.2:1 contrast.
* **Solution**:
  - Update secondary text styling in `TextStyle` and `DropFileWidget` to use `Colors::Grey600` or `Grey700` (achieving ≥ 4.5:1 AA contrast ratio).

---

## Phase 4: PDF ↔ Image Cross-Conversion

### 4.1 — Images → PDF (Merge)
* Convert multiple selected image files into a single consolidated multi-page PDF using `QPdfWriter` + `QPainter::drawImage()`.
* Configurable page size (A4 / Fit to original) and page orientation.

### 4.2 — PDF → Images (Extraction)
* Extract every page of an input PDF into standalone image files (`PNG`, `JPG`, `WEBP`) using `QPdfDocument::render()`.
* Configurable render resolution (150 DPI, 300 DPI, 600 DPI).

### 4.3 — PDF Compression Presets
* One-click compression presets in `PdfPage`:
  - **Web / Email**: 72 DPI, 0.5× scale, Indexed8 (~70-80% reduction)
  - **Balanced**: 150 DPI, 1.0× scale, RGB888 (~40-50% reduction)
  - **High Quality**: 200 DPI, 2.0× scale, RGB888 (~10-20% reduction)
