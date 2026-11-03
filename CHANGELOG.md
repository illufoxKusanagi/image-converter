# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2026-11-02

### Added
- **Complete PDF Toolkit**:
  - **Merge PDF**: Multi-PDF combining with drag-and-drop file reordering, page validation, and atomic output saving.
  - **Split PDF**: Splitting by custom page ranges (`1-3, 5`), fixed interval (every `N` pages), or extracting all pages into separate PDFs.
  - **Organize / Rotate / Delete PDF Pages**: Visual page organizer with rendered page thumbnails, 90°/180°/270° per-page rotation, page reordering, and deletion.
  - **PDF to Images**: High-resolution page extractor supporting PNG, JPG, WEBP, and TIFF at 72–600 DPI with white-paper compositing.
  - **Images to Merged PDF**: Multi-image to PDF combiner with `Fit Image`, `A4`, and `Letter` page sizes, auto/portrait/landscape orientation, margins, and EXIF orientation handling.
  - **PDF to Grayscale**: Single and batch color-to-grayscale conversion via Ghostscript or Qt PDF fallback.
  - **Segmented Tool Selector**: Tool switching across all 7 PDF operations with dynamic drop zone swapping (`PDF` vs `IMAGE`).
- **Standalone UI-Kit Distribution & Demo**:
  - Comprehensive `ui-kit/AI_AGENT_GUIDE.md` for integrating `ui-kit` into external C++17 Qt 6 / Qt 5 projects.
  - Standalone `ui-kit/README.md` and dedicated `ui_kit_demo` executable showcase with top-bar theme toggle and `DemoPage`.
- **Expanded UI-Kit Components (Wave 2)**:
  - Added `Alert`, `Checkbox` (with tri-state & error validation), `Combobox` (searchable popup), `EmptyState`, `Menu` (Dropdown & Context menu), `NumberInput` / `DecimalInput`, `Popover`, `RadioGroup`, `ScrollArea`, `Splitter`, and `Textarea`.

### Changed
- Main window navigation tab renamed from "Compress PDF" to "PDF Tools".
- Streamlined main window navigation by focusing on core conversion workflows and providing the standalone demo executable for component testing.

### Fixed
- `NumberInput` & `DecimalInput`: Styled stepper buttons with compartment borders, hover/pressed feedback, and Lucide SVG icons.
- `Checkbox`: Enhanced contrast of unchecked indicator border for light and dark themes.
- `Toast`: Scoped stylesheets with `Qt::WA_StyledBackground` to prevent borders cascading onto child labels.
- `ui::Tabs`: Sized height to header-bar content when used in header-only mode without child pages.

## [1.1.0] - 2026-11-01

### Added
- **Ghostscript Compression Pipeline**: High-fidelity PDF compression engine (`pdfwrite`) with distiller parameters, JPEG QFactor calculations, downsampling options, atomic file saving with `QSaveFile`, and presets (`Custom`, `Web / Email`, `Balanced`, `High Quality` with Flate preservation).
- **Real-Time DropZone Status**: Added live per-file conversion state machine to `DropFileWidget` (`Waiting`, `Processing`, `Succeeded`, `Unchanged`, `Failed`, `Cancelled`) with inline animated `ui::Spinner` and status label on each file card.
- **Flattened PDF to JPG Conversion**: Composited rendered PDF pages onto white paper to eliminate black/transparent background rendering.
- Cross-platform CI/CD packaging support for Fedora/RHEL (`.rpm`) and Windows portable `.zip`.

## [1.0.0] - 2026-08-30

### Added
- **Batch Image Converter**: Multi-format conversion between `JPG`, `JPEG`, `PNG`, `WEBP`, `TIFF`, `BMP`, `GIF`, and `PDF` with format-aware quality and asynchronous background execution.
- **Multi-File Queue**: Drag-and-drop drop zone with scrollable queue, "Add More", "Clear All", per-item remove actions, and formatted file size displays.
- **Cross-Format Conversions**: Images to multi-page PDF (`QPdfWriter` + `QPainter`) and PDF to images (`QPdfDocument::render`).
- **Shadcn-Inspired Qt UI-Kit (Wave 1)**: Initial suite of reusable Qt Widgets components (`Avatar`, `Button`, `Badge`, `Progress`, `Slider`, `Select`, `Switch`, `FormField`, `Card`, `Separator`, `Table`, `Input`) with reactive `ui::Theme` (Light/Dark/System) and 20+ bundled Lucide SVG icons.
- **Component Gallery**: Built-in showcase page for exploring and testing UI components interactively.
- Automated Debian/Ubuntu (`.deb`) packaging and release pipeline via GitHub Actions.

[Unreleased]: https://github.com/illufoxKusanagi/image-converter/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/illufoxKusanagi/image-converter/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/illufoxKusanagi/image-converter/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/illufoxKusanagi/image-converter/releases/tag/v1.0.0
