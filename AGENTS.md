# ImageConverter — Agent Workspace Context & Instructions

This file serves as the persistent root knowledge and instructions for Antigravity agents working on the **ImageConverter** project.

---

## 1. Project Mission & Identity
**ImageConverter** is a native, offline C++ desktop application built on the **Qt 6 / Qt 5** framework. Its core purposes are:
1. **Image Conversion**: Fast, local conversion between common image formats (`JPG`, `JPEG`, `PNG`, `WEBP`, `TIFF`, `BMP`, `GIF`, `PDF`) with user-configurable quality compression.
2. **PDF Compression**: Local multi-page PDF compression through resolution reduction, downscaling, and indexed/RGB color quantization using `QPdfDocument` and `QPdfWriter`.

---

## 2. Core Tech Stack & Build Tooling
- **Language**: C++17 (`CMAKE_CXX_STANDARD 17`)
- **Framework**: Qt 6 (Widgets, Core, Pdf, Svg) with fallback support for Qt 5
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
├── ui-kit/                   # Qt C++ "Shadcn-Style" component library (static library)
│   ├── theme/                # Tokens, Theme manager, StyleHelper, AnimationHelper, Icon helper
│   └── components/           # Button, Badge, Spinner, Card, Input, Switch, Slider, Select, FormField, Tabs, etc.
├── mainwindow/               # Shell window & tab management
│   └── mainwindow.h/.cpp     # Main application window with ui::Tabs and theme switch
├── pages/                    # Main application view pages
│   ├── main_page.h/.cpp      # Image conversion UI & batch orchestration
│   └── pdf_page.h/.cpp       # PDF compression UI & rasterization engine
├── widgets/                  # Reusable custom-styled Qt widgets
│   ├── drop_file_widget.*    # Drag-and-drop target zone & image saving logic
│   ├── slider_widget.*       # Synchronized slider + spinbox quality selector
│   ├── button_action.*       # Primary styled CTA buttons
│   ├── input_widget.*        # Compound input containers
│   └── message_box_widget.*  # Styled modal alert dialogs
├── resources/                # Assets and icon definitions
│   ├── icons.qrc             # Resource manifest (includes official Lucide SVGs)
│   └── icons/                # Lucide SVG vector assets
├── CMakeLists.txt            # CMake build definition
└── main.cpp                  # Application entry point & Qt event loop
```

---

## 4. Strict Agent Behavioral Guardrails & Anti-Autopilot Policy
These rules are non-negotiable and take absolute precedence over default model behaviors:

1. **Strict Single-Task Scope Lock**:
   - **NEVER** modify or create files that were not explicitly named or requested in the user's prompt.
   - **NEVER** advance to the next roadmap milestone, phase, or unrequested component automatically.
   - **NEVER** batch multiple unrelated features or files in one turn. Execute *only* the immediate task and stop.
2. **Zero-Tolerance for Faked or Assumed Status**:
   - **NEVER** claim a build compiled or tests passed without actually executing `cmake --build` via `run_command` and verifying exit code `0`.
   - **NEVER** claim CodeRabbit review passed without actually running `cr review --agent --uncommitted` and inspecting the real JSON output (`findings: 0`).
   - **ALWAYS** provide dual validation: verify programmatic execution internally AND present the raw terminal exit code / proof in the user response.
3. **Non-Zero File Size Verification**:
   - Every file created or edited MUST be audited to ensure its byte length is greater than zero (`Length > 0`). Empty or truncated files are strictly unacceptable.
4. **No Presumptive Approvals**:
   - Conversational approvals like *"ok"*, *"go ahead"*, or *"proceed"* apply **ONLY** to the immediate single task under discussion. Never interpret them as blanket permission to start future roadmap phases.
5. **Clarify-Before-Action on Ambiguity**:
   - If a user prompt is underspecified, ambiguous, or lacks an exact file/function target, **STOP** and ask 1 brief clarifying question. Do not guess, assume intent, or write speculative code.
6. **Mandatory Immediate Tool Stop**:
   - After completing the single requested edit, verifying non-zero file sizes, and confirming the build, **STOP calling tools immediately** and summarize the result concisely. Do not ask leading questions to push the roadmap forward.

---

## 5. Key Agent Technical Directives & Rules
When analyzing, modifying, or creating code in this repository, follow these technical standards:

1. **Design System Adherence**:
   - Never hardcode arbitrary colors or fonts in CSS/QSS strings.
   - Always reference `ui::Theme::instance().colors()` or `ui::StyleHelper`.
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

---

## 6. Automated Code Review with CodeRabbit CLI

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
5. Report the review outcome and verified fixes to the user with raw JSON proof.