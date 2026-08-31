# ImageConverter — Improvement Roadmap & Audit Log (v3.2 — All Phases Complete)

> Grounded in full codebase audit and reviewed against `qt-cpp-review` & `qt-ui-design` standards.

---

## 📊 Priority Matrix & Status

| Phase       | Milestone                          | Focus Areas                                                               | Complexity |   Status   |
| :---------- | :--------------------------------- | :------------------------------------------------------------------------ | :--------- | :--------: |
| **Phase 0** | **Bug Fixes & Code Cleanup**       | File dialog filters, hardcoded colors, dead code, uninitialized variables | Small      | 🟢 **DONE** |
| **Phase 1** | **Async Execution & Concurrency**  | `QtConcurrent` workers, progress bar, cancel flag, const/API polish       | Medium     | 🟢 **DONE** |
| **Phase 2** | **Format-Aware Quality & Formats** | PNG lossless handling, BMP/GIF support, compression stats toast           | Small      | 🟢 **DONE** |
| **Phase 3** | **File List & UI Scalability**     | Scrollable file list, drag hover states, High-DPI & WCAG contrast fix     | Medium     | 🟢 **DONE** |
| **Phase 4** | **PDF ↔ Image Suite**              | Images→PDF merge, PDF→Images extraction, PDF compression presets          | Medium     | 🟢 **DONE** |
| **Phase 5** | **Advanced Power Tools**           | Configurable DPI extraction, image reordering sequence controls           | Medium     | 🟢 **DONE** |

---

## 🔍 Audit & Review Findings Tracking

| Issue ID    | Domain          | Finding / Warning                                                          |          Status          |
| :---------- | :-------------- | :------------------------------------------------------------------------- | :----------------------: |
| **[D-001]** | `qt-cpp-review` | Synchronous batch and PDF processing on main GUI event loop                |  🟢 **Resolved in 1.1**   |
| **[D-002]** | `qt-cpp-review` | Non-idiomatic getter prefixes (`getValue()`, `getFilePath()`, `getText()`) |  🟢 **Resolved in 1.4**   |
| **[D-003]** | `qt-cpp-review` | Missing `const` qualifiers on read-only accessor methods                   |  🟢 **Resolved in 1.4**   |
| **[D-004]** | `qt-cpp-review` | Pass-by-value `QString` in `convertImage()`                                |  🟢 **Resolved in 1.4**   |
| **[U-001]** | `qt-ui-design`  | Doherty threshold: >400ms operations lack visual progress indicator        |  🟢 **Resolved in 1.2**   |
| **[U-002]** | `qt-ui-design`  | WCAG 2.2 AA Contrast: `Grey400` on white is ~3.2:1 (<4.5:1 required)       |  🟢 **Resolved in 3.4**   |
| **[U-003]** | `qt-ui-design`  | Fixed 352px width causes long filenames to wrap awkwardly under High-DPI   |  🟢 **Resolved in 3.3**   |
| **[CR-01]** | `coderabbit`    | Incomplete PDF file remaining on disk when cancelled / failed              | 🟢 **Resolved in Review** |
| **[CR-02]** | `coderabbit`    | Batch conversion filename collisions overwriting earlier outputs           | 🟢 **Resolved in Review** |
| **[CR-03]** | `coderabbit`    | Dotted directory path causing extension suffix stripping truncation        | 🟢 **Resolved in Review** |
| **[CR-04]** | `coderabbit`    | Skewed batch reduction percentage on failed/corrupted files                | 🟢 **Resolved in Review** |
| **[CR-05]** | `coderabbit`    | Pre-existing destination file destruction on cancel/failure                | 🟢 **Resolved in Review** |

---

## Phase 0: Bug Fixes & Code Cleanup ✅ COMPLETED

- [x] **0.1 Fixed Swapped File Dialog Filters**: PDF mode shows `"PDF Files (*.pdf)"`, Images shows `"Image Files (*.png *.jpg ...)"` ([`widgets/drop_file_widget.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/widgets/drop_file_widget.cpp)).
- [x] **0.2 Dynamic Dropzone Format Label**: PDF page shows `"pdf"`; Image page shows `"jpg, jpeg, png, webp, tiff, bmp, gif"`.
- [x] **0.3 Silent Dialog Cancel**: Cancelling file dialogs returns cleanly without showing error popups.
- [x] **0.4 Design System Compliance**: Replaced all hardcoded hex strings (`#d1d5db`, `#ffffff`, `#f5f6fa`) with `Colors::*` tokens.
- [x] **0.5 Semantic Naming**: Renamed `m_sourceExtension` to `m_targetImageExtension`.
- [x] **0.6 Fixed PDF Multi-Page Sizing**: Re-applied `pdfWriter.setPageSize()` per page in [`pages/pdf_page.cpp`](file:///d:/matkul/sem_6/AppProject/ImageConverter/pages/pdf_page.cpp).
- [x] **0.7 Corrected Typos & Resources**: Fixed `Subttile*` -> `Subtitle*` in [`styles/text_style.h`](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/text_style.h); registered category icons in [`resources/icons.qrc`](file:///d:/matkul/sem_6/AppProject/ImageConverter/resources/icons.qrc).
- [x] **0.8 Fixed Uninitialized Variables & Static Bug**: Initialized `value` in `InputType`, `m_inputValue(0.0)` in `InputWidget`, removed static variable state corruption in `ButtonSidebarWidget`.
- [x] **0.9 Removed Dead Code & Stubs**: Removed empty stubs, unreferenced `mainwindow_dummy.*`, and empty `mainwindow.ui`.

---

## Phase 1: Async Concurrency & Progress ✅ COMPLETED

- [x] **1.1 Non-Blocking Background Workers (`QtConcurrent`)**:
  - Image conversion (single & batch) and multi-page PDF compression execute asynchronously in thread pool workers.
- [x] **1.2 Interactive Progress Bar (`QProgressBar`)**:
  - Progress tracking for batch images (`Converting %v of %m images`) and multi-page PDF (`Compressing page %v of %m`).
- [x] **1.3 Cancellation Support**:
  - `std::atomic<bool> m_isCancelled` gracefully aborts background loops and cleans up resources without corrupted partial writes.
- [x] **1.4 C++ API & Const-Correctness Polish**:
  - Added const qualifiers and idiomatic property accessors across `SliderWidget`, `InputWidget`, `InputDropdown`, `ButtonAction`, and `DropFileWidget`.

---

## Phase 2: Format-Aware Quality & Extended Formats ✅ COMPLETED

- [x] **2.1 Smart Quality Slider Behavior**:
  - Lossless PNG (`-1`), uncompressed BMP, indexed GIF, and adjustable JPG/JPEG/WEBP/TIFF (0–100).
- [x] **2.2 Extended BMP and GIF Support**:
  - Full support across dropdowns, open dialogs, save handlers, and drag-and-drop MIME checkers.
- [x] **2.3 Compression Statistics Summary**:
  - Real-time before vs after file size comparison and percentage reduction reporting.

---

## Phase 3: File List & UI Scalability ✅ COMPLETED

- [x] **3.1 Scrollable Multi-File List View**:
  - Item cards with format icon, elided filename, size badge, and per-item remove `[✕]` button.
  - Header summary (`"3 files selected • 4.8 MB"`) + `"Clear All"` reset button + `"+ Add More Files"` append button.
- [x] **3.2 Interactive Drag Hover Feedback**:
  - Dynamic visual state changes: `2px dashed Colors::Primary500` + `Colors::Secondary50` background tint on hover.
- [x] **3.3 Flexible Window Width & High-DPI Support**:
  - Widened tab widget to 460px with 180px tabs and 320px action buttons.
- [x] **3.4 WCAG 2.2 AA Contrast Compliance**:
  - Upgraded text tokens to `Colors::Grey700` (`#41444C`) and `Colors::Grey800` (`#2C2D33`) for contrast ratios ≥ 7:1.

---

## Phase 4: PDF ↔ Image Cross-Conversion Suite ✅ COMPLETED

- [x] **4.1 Images → PDF (Single & Multi-Image Merge)**:
  - Added `"pdf"` target format to `MainPage`.
  - Single image: Converts image to a high-resolution 1-page PDF.
  - Multiple images: Merges selected images in sequential order into a single consolidated multi-page PDF with per-image aspect ratio sizing.
- [x] **4.2 PDF → Images (Extraction)**:
  - When `.pdf` file is dropped into `MainPage` and target is an image format (`JPG`, `PNG`, `WEBP`, `TIFF`, `BMP`, `GIF`), extracts all pages into standalone numbered images (`doc_page1.png`, `doc_page2.png`, etc.) at 150 DPI.
- [x] **4.3 PDF Compression Presets**:
  - Added one-click compression preset dropdown in `PdfPage`:
    - **Web / Email**: Quality 20 (70–80% size reduction)
    - **Balanced**: Quality 50 (40–50% size reduction)
    - **High Quality**: Quality 80 (15–25% size reduction)
    - **Custom**: Synchronized directly with the manual quality slider.
- [x] **4.4 Atomic File Safety**:
  - PDF writes use temporary files (`.tmp`) and atomically replace destinations upon 100% successful completion, protecting existing user files on cancel/failure.

---

## Phase 5: Advanced Power Tools & Workflow Polish ✅ COMPLETED

- [x] **5.1 Multi-File Reordering Sequence Controls**:
  - Interactive Move Up `▲` and Move Down `▼` buttons on file cards in `DropFileWidget` to arrange the exact page sequence before merging into a multi-page PDF.
- [x] **5.2 Configurable PDF Extraction Resolution**:
  - Dynamic resolution scaling ranging from `72 DPI (Web / Fast)` to `600 DPI (Ultra-HD / Archival)` controlled seamlessly via the quality slider.

---

## 🎨 Future Track: Qt C++ "Shadcn-Style" Component Library (`ui-kit/`)

| Phase       | Module                          | Components & Capabilities                                                                                                      |    Status     |
| :---------- | :------------------------------ | :----------------------------------------------------------------------------------------------------------------------------- | :-----------: |
| **Phase 0** | **Foundation & Tooling (P0)**   | `ui_kit` CMake target, `Theme` / `Tokens` system (Light/Dark), `Fusion` base style, Animation & SVG helpers, Storybook Gallery | ⏳ **Planned** |
| **Phase 1** | **Core Primitives (P1)**        | `ui::Button` (variants/sizes/spinner), `ui::Badge`, `ui::Spinner`, `ui::Card` (drop shadow)                                    | ⏳ **Planned** |
| **Phase 2** | **Inputs & Forms (P2)**         | `ui::Input` (leading/clear icon), `ui::Switch`, `ui::Slider`, `ui::Checkbox`/`ui::Radio`, `ui::Select`                         | ⏳ **Planned** |
| **Phase 3** | **Layout & Navigation (P2/P3)** | `ui::Sidebar` (collapsible/animated), `ui::Navbar`, `ui::Tabs`, `ui::Breadcrumb`, `ui::Accordion`                              | ⏳ **Planned** |
| **Phase 4** | **Feedback & Overlays (P3)**    | `ui::Toast` (stacking queue), `ui::Dialog` (backdrop dim), `ui::ProgressBar`, `ui::Skeleton` (shimmer sweep)                   | ⏳ **Planned** |
| **Phase 5** | **Content & Data (P4)**         | `ui::Avatar`, `ui::ItemDelegate`, `ui::DatePicker`                                                                             | ⏳ **Planned** |

