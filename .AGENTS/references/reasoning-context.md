# Reasoning Context & Architectural Decisions

## 1. Project Mental Model & Purpose
ImageConverter was created as a high-performance, lightweight, self-contained desktop utility for end users who need to convert images and compress PDF files quickly without relying on cloud-based converters, privacy-invasive upload services, or network connectivity.

---

## 2. Core Architectural Decisions & Rationale

### 2.1 Why Pure Qt Widgets over QML / Web Tech
- **Decision**: The project is implemented using C++ with `QtWidgets` and customized stylesheets (QSS), rather than QML or Electron/WebView.
- **Rationale**:
  - Direct low-overhead access to native OS file dialogs and low-level image processing.
  - Zero web-engine overhead: fast startup (< 100ms), low memory footprint (< 30MB RAM).
  - Clean portable single-binary distribution on Windows without heavy runtime dependencies.

### 2.2 Why QPdfDocument + QPdfWriter for PDF Compression
- **Decision**: Rasterize and re-encode pages rather than modifying PDF stream dictionary objects directly.
- **Rationale**:
  - `Qt6::Pdf` provides robust rendering via `QPdfDocument` and page generation via `QPdfWriter` out of the box without requiring massive third-party PDF SDKs like Ghostscript, Poppler, or PDFium C++ wrappers.
  - Granular control over resolution (DPI scaling) and color quantization (`Format_Indexed8`, `Format_RGB888`) enables predictable file size reduction across arbitrary PDF inputs containing mixed text and raster elements.

### 2.3 Image Format Handling Decisions
- **PNG Quality**:
  - `QImage::save(..., "PNG", -1)`: PNG compression is lossless (zlib deflation level); quality arguments in `QImage` for PNG control zlib compression levels (0-9 / default -1), not lossy visual fidelity. Therefore, the slider's 0-100 visual quality scale is not applied as a lossy factor to PNGs to avoid unexpected artifacts.
- **JPG / JPEG / WEBP / TIFF**:
  - These formats support lossy compression, where the `quality` integer (0–100) directly dictates DCT / quantization matrix compression in Qt's underlying image plugins.

### 2.4 State Management & Signal Feedback Loops
- **Bidirectional Controls (`SliderWidget`)**:
  - When a slider moves, the spinbox updates. When the spinbox value changes, the slider position moves.
  - **Protection**: `blockSignals(true)` / `blockSignals(false)` is mandatory during programmatic value synchronization to prevent infinite recursive signaling and stack overflow.

---

## 3. UI State Transitions & Interaction Flows

### 3.1 Drop Zone State Machine (`DropFileWidget`)
```
[EMPTY STATE: m_emptyFieldWidget visible]
       |
       | User drags valid files OR selects via QFileDialog
       v
[FILE SELECTED STATE: m_chosenFileWidget visible]
       |
       | Single file: shows filename
       | Multiple files: shows "N files selected"
       |
       | User triggers conversion
       v
[CONVERTING / SAVING]
       |
       | Single: Prompt save file path -> save -> QMessageBox
       | Batch: Prompt output folder -> iterate -> save all -> Batch Summary QMessageBox
```

### 3.2 Dynamic Input Widget Factory (`InputWidget`)
`InputWidget` is designed as a polymorphic input wrapper based on `InputType.type`:
- `"dropdown"` -> instantiates `InputDropdown`
- `"field"` -> instantiates `InputField`
- `"upload"` -> instantiates `InputUpload`
- other -> instantiates `InputInvalid` (informative diagnostic widget)

This pattern allows configuration of input controls declaratively.

---

## 4. Known Edge Cases & Handling Protocols

1. **Null / Corrupted Image Inputs**:
   - Always checked via `image.isNull()`.
   - In single-file mode: displays `MessageBoxWidget::Critical` with informative text.
   - In batch mode: increments `failureCount` and continues processing remaining images without crashing.
2. **Cancelled File Dialogs**:
   - If `QFileDialog::getSaveFileName()` or `getExistingDirectory()` returns an empty `QString`, operation aborts safely without error or partial file write.
3. **Invalid PDF Documents**:
   - `QPdfDocument::load()` returns an error enum. Checked against `QPdfDocument::Error::None` before proceeding to render loop.
4. **Window Resizing Constraints**:
   - Window size is constrained using fixed dialog hints and layout fixed widths (`m_tabWidget->setFixedWidth(352)`) to ensure pixel-perfect layout alignment across different display DPI settings.
