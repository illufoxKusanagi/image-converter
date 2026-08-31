# ImageConverter — Agent Workspace Context & Instructions

This file serves as the persistent root knowledge and instructions for Antigravity agents working on the **ImageConverter** project.

---

## 1. Project Mission & Identity
**ImageConverter** is a native, offline C++ desktop application built on the **Qt 6 / Qt 5** framework. Its core purposes are:
1. **Image Conversion**: Fast, local conversion between common image formats (`JPG`, `JPEG`, `PNG`, `WEBP`, `TIFF`) with user-configurable quality compression.
2. **PDF Compression**: Local multi-page PDF compression through resolution reduction, downscaling, and indexed/RGB color quantization using `QPdfDocument` and `QPdfWriter`.

---

## 2. Core Tech Stack & Build Tooling
- **Language**: C++17 (`CMAKE_CXX_STANDARD 17`)
- **Framework**: Qt 6 (Widgets, Core, Pdf) with fallback support for Qt 5
- **Build System**: CMake 3.16+ with Ninja / MinGW (LLVM-MinGW 17.0.6 64-bit or MinGW 64-bit)
- **Asset Bundler**: Qt Resource System (`resources/icons.qrc` compiled via `CMAKE_AUTORCC`)
- **No External Web Dependencies**: Strictly offline, zero tracking, lightweight.

---

## 3. Directory Layout & Architecture
```
ImageConverter/
├── .AGENTS/                  # Workspace knowledge, rules, skills, and reasoning context
│   ├── rules/                # Hierarchical domain rules (architecture, UI, pipeline, coding standards)
│   ├── skills/               # Interactive procedures and developer workflows
│   └── references/           # Complete codebase map, symbol registry, and reasoning context
├── mainwindow/               # Shell window & tab management
│   ├── mainwindow.h/.cpp     # Main application window with customized QTabWidget
│   └── mainwindow_dummy.*    # Legacy prototype (for reference only)
├── pages/                    # Main application view pages
│   ├── main_page.h/.cpp      # Image conversion UI & batch orchestration
│   └── pdf_page.h/.cpp       # PDF compression UI & rasterization engine
├── widgets/                  # Reusable custom-styled Qt widgets
│   ├── drop_file_widget.*    # Drag-and-drop target zone & image saving logic
│   ├── slider_widget.*       # Synchronized slider + spinbox quality selector
│   ├── button_action.*       # Primary styled CTA buttons
│   ├── input_widget.*        # Compound input containers
│   ├── input_dropdown.*      # Styled dropdown format picker
│   ├── message_box_widget.*  # Styled modal alert dialogs
│   └── sidebar_panel.*       # Collapsible navigation drawer
├── styles/                   # Centralized design tokens
│   ├── colors.h              # HSL-based palette (Primary, Secondary, Grey, Danger, Warning)
│   └── text_style.h          # Roboto typography helper methods
├── resources/                # Assets and icon definitions
│   ├── icons.qrc             # Resource manifest
│   ├── icon_type.h           # Icon name-to-path resolution map
│   └── input_type.h          # Type descriptor for dynamic input widgets
├── CMakeLists.txt            # CMake build definition
└── main.cpp                  # Application entry point & Qt event loop
```

---

## 4. Key Agent Directives & Rules
When analyzing, modifying, or creating code in this repository, follow these rules:

1. **Design System Adherence**:
   - Never hardcode arbitrary colors or fonts in CSS/QSS strings.
   - Always reference `Colors::*` (from [styles/colors.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/colors.h)) and `TextStyle::*` (from [styles/text_style.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/text_style.h)).
2. **Memory & Qt Hierarchy**:
   - Always pass `parent` pointers to `QObject` and `QWidget` constructors to allow Qt's parent-child ownership tree to handle automatic memory cleanup.
3. **Signal/Slot Discipline**:
   - Prevent recursive signal loops (e.g. `SliderWidget`) using `blockSignals(true)` / `blockSignals(false)` when synchronizing linked widgets.
4. **Format & Conversion Rules**:
   - `PNG` uses lossless compression; quality parameter passed to `QImage::save` should be `-1`.
   - `JPG`, `JPEG`, `WEBP`, `TIFF` accept quality values `0–100`.
   - Always check `QImage::isNull()` or `QPdfDocument::Error` before processing files.
5. **Context Retention**:
   - Refer to detailed documentation in `.AGENTS/rules/`, `.AGENTS/skills/`, and `.AGENTS/references/` before making architectural decisions.

## 5. Automated Code Review with CodeRabbit CLI

### Exact Command
```powershell
cr review --agent --uncommitted
```
*(Alias for `coderabbit review --agent --uncommitted`)*

### When You MUST Run CodeRabbit Review
- **Feature Completion**: After implementing a new functional feature or significant capability.
- **Bug Fixes & Refactoring**: After fixing bugs, race conditions, memory issues, or architectural refactors.
- **Multi-Phase Milestones**: After completing multi-phase code changes before presenting final results to the user.

### When You MUST NOT Run CodeRabbit Review
- **File Renames / Moves**: Pure file renaming or reorganization without substantive logic changes.
- **CI / GitHub Actions**: Edits to `.github/workflows/`, CI configurations, or build script metadata.
- **Documentation Only**: Updating `.md` documents, comments, README files, or roadmap tracking logs.
- **Trivial Tweaks**: Single-line typo corrections, whitespace formatting, or minor variable renames.

### Review Execution & Fix Loop
1. Launch `cr review --agent --uncommitted` as a background task.
2. Check task progress periodically until complete.
3. Review and validate findings:
   - **Fix**: Critical / Major issues (data safety, memory leaks, invalid state handling).
   - **Fix**: Recommended / Minor logic issues.
   - **Skip**: Trivial nits or unnecessary changes with a brief rationale.
4. Apply the fixes, re-compile, and re-run `cr review --agent --uncommitted` (up to 3 validation loops).
5. Report the review outcome and verified fixes to the user.