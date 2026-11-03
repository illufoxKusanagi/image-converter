# Qt UI-Kit (`ui-kit`) — AI Agent Integration & Operational Guide

This document provides exact, machine-actionable instructions, architectural contracts, and API signatures for AI coding agents integrating **`ui-kit`** into any C++17 Qt 6 or Qt 5 desktop application.

---

## 1. Architecture & Directory Overview

`ui-kit` is a self-contained static library (`ui_kit`) built on `Qt::Core`, `Qt::Widgets`, and `Qt::Svg`. All symbols live inside `namespace ui`.

```text
ui-kit/
├── CMakeLists.txt              # Static library target `ui_kit` + optional `ui_kit_demo`
├── README.md                   # Human & developer reference documentation
├── AI_AGENT_GUIDE.md           # This operational guide for AI agents
├── theme/                      # Centralized design tokens, theme singleton, & helpers
│   ├── tokens.h                # ColorTokens, RadiusTokens, SpacingTokens, TypographyTokens, ThemeMode
│   ├── theme.h / theme.cpp     # Singleton `ui::Theme::instance()` with `themeChanged` signal
│   ├── style_helper.h          # QColor/QSS utilities (`toHexString`, `toRgbaString`, `tint`, `hoverColor`)
│   ├── animation_helper.h      # Standard durations (`DurationFast`, `DurationNormal`) & easing curves
│   └── icon_helper.h/.cpp      # Dynamic SVG color tinting (`ui::Icon::get`, `ui::Icon::pixmap`)
├── components/                 # 30 self-contained Shadcn-inspired Qt Widgets components
│   ├── accordion.h/.cpp        # Collapsible section with animated height
│   ├── alert.h/.cpp            # Persistent inline status banner (Info, Success, Warning, Destructive)
│   ├── avatar.h/.cpp           # Circular user/entity badge with initials, image, and status dot
│   ├── badge.h/.cpp            # Pill-shaped category/status tag
│   ├── button.h/.cpp           # Push button with 6 variants, 4 sizes, and built-in loading spinner
│   ├── card.h/.cpp             # Elevated surface container with header, content, and footer layouts
│   ├── checkbox.h/.cpp         # Custom-painted checkbox with tri-state & error state support
│   ├── combobox.h/.cpp         # Searchable dropdown selector with filterable popover list
│   ├── dialog.h/.cpp           # Modal dialog with scrollable description & static confirm/info helpers
│   ├── empty_state.h/.cpp      # Placeholder panel with icon, title, description, and optional CTA
│   ├── form_field.h/.cpp       # Wrapper adding label, required indicator, description, and error text
│   ├── input.h/.cpp            # Single-line text input with prefix/suffix labels, clear button, & error state
│   ├── menu.h/.cpp             # Themed QMenu (`ui::Menu`, `ui::DropdownMenu`, `ui::ContextMenu`)
│   ├── number_input.h/.cpp     # Integer (`ui::NumberInput`) & floating-point (`ui::DecimalInput`) spinboxes
│   ├── popover.h/.cpp          # Anchored floating container with automatic outside-click/Escape dismissal
│   ├── progress.h/.cpp         # Animated progress bar with determinate & indeterminate modes
│   ├── radio_group.h/.cpp      # Exclusive radio button group with vertical/horizontal orientation
│   ├── scroll_area.h/.cpp      # Custom-styled QScrollArea with minimalist themed scrollbars
│   ├── select.h/.cpp           # Non-searchable themed QComboBox
│   ├── separator.h/.cpp        # 1px horizontal or vertical divider line
│   ├── sidebar.h/.cpp          # Collapsible vertical navigation rail with icon + label items
│   ├── slider.h/.cpp           # Themed horizontal/vertical range slider
│   ├── spinner.h/.cpp          # Animated vector arc loading indicator
│   ├── splitter.h/.cpp         # Resizable multi-pane QSplitter with focusable handle indicator
│   ├── switch.h/.cpp           # Animated toggle switch (`QAbstractButton` subclass)
│   ├── table.h/.cpp            # Themed QTableWidget with row selection and flat header styling
│   ├── tabs.h/.cpp             # Tab bar + stacked page container (`Underline` and `Segmented` variants)
│   ├── textarea.h/.cpp         # Multi-line plain-text editor (`QPlainTextEdit`) with error state
│   ├── toast.h/.cpp            # Self-dismissing floating notification overlay
│   └── tooltip.h/.cpp          # Global QToolTip stylesheet installer & widget helper
└── demo/                       # Interactive component showcase
    ├── demo_page.h/.cpp        # Reusable `ui::DemoPage` widget demonstrating all 30 components
    └── main.cpp                # Standalone showcase entry point (`ui_kit_demo`)
```

---

## 2. Step-by-Step CMake Integration

### Option A: Local Subdirectory or Git Submodule
Place `ui-kit/` inside the host repository and add it in the root `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg)

# 1. Include ui-kit subdirectory
add_subdirectory(ui-kit)

# 2. Define your executable (include icons.qrc if using Select/NumberInput arrows)
add_executable(MyApp
    main.cpp
    resources/icons.qrc
)

# 3. Link against ui_kit (transitively links Qt::Core, Qt::Widgets, Qt::Svg)
target_link_libraries(MyApp PRIVATE ui_kit)
```

### Option B: CMake `FetchContent`
```cmake
include(FetchContent)
FetchContent_Declare(
    ui_kit
    GIT_REPOSITORY https://github.com/your-org/ui-kit.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(ui_kit)

target_link_libraries(MyApp PRIVATE ui_kit)
```

### Header Include Conventions
`ui_kit` exports `${CMAKE_CURRENT_SOURCE_DIR}` (`ui-kit/`) as a `PUBLIC` include directory:
- Always work: `#include "components/button.h"`, `#include "theme/theme.h"`, `#include "demo/demo_page.h"`
- If the parent repository root is also on the include path: `#include "ui-kit/components/button.h"`

---

## 3. Required Icon Resources (`icons.qrc`)

### Arrow Icons Used by `Select`, `NumberInput`, and `DecimalInput`
 Qt stylesheets (`QComboBox::down-arrow`, `QSpinBox::up-arrow`, `QSpinBox::down-arrow`) load vector arrows via Qt Resource (`qrc`) paths depending on whether the active theme is dark or light:

| Resource Path | Theme Mode | Usage |
| :--- | :--- | :--- |
| `:/icons/icons/arrow-up.svg` | Dark (`isDark() == true`) | `ui::NumberInput`, `ui::DecimalInput` up arrow (light stroke) |
| `:/icons/icons/arrow-up-black.svg` | Light (`isDark() == false`) | `ui::NumberInput`, `ui::DecimalInput` up arrow (dark stroke) |
| `:/icons/icons/arrow-down.svg` | Dark (`isDark() == true`) | `ui::Select`, `ui::NumberInput`, `ui::DecimalInput` down arrow (light stroke) |
| `:/icons/icons/arrow-down-black.svg` | Light (`isDark() == false`) | `ui::Select`, `ui::NumberInput`, `ui::DecimalInput` down arrow (dark stroke) |

Ensure your application's `.qrc` file registers these 4 paths under `<qresource prefix="/icons">`:
```xml
<RCC>
    <qresource prefix="/icons">
        <file>icons/arrow-up.svg</file>
        <file>icons/arrow-up-black.svg</file>
        <file>icons/arrow-down.svg</file>
        <file>icons/arrow-down-black.svg</file>
    </qresource>
</RCC>
```

### Dynamic SVG Tinting with `ui::Icon`
For all other icons (buttons, tabs, sidebar items, empty states), use `ui::Icon` (`theme/icon_helper.h`) to dynamically recolor any SVG resource at runtime using semantic theme tokens:

```cpp
#include "theme/icon_helper.h"
#include "theme/theme.h"

const auto &c = ui::Theme::instance().colors();
QIcon folderIcon = ui::Icon::get(":/icons/icons/folder.svg", c.foreground, QSize(18, 18));
myButton->setIcon(folderIcon);
```

---

## 4. Theme Initialization & Reactive Widget Pattern (Mandatory)

### Initializing `ui::Theme` in `main.cpp`
Initialize `ui::Theme::instance()` right after constructing `QApplication`:

```cpp
#include <QApplication>
#include "theme/theme.h"
#include "components/tooltip.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Set initial theme mode: ThemeMode::System, ThemeMode::Light, or ThemeMode::Dark
  ui::Theme::instance().setMode(ui::ThemeMode::System);

  // Optional brand customization:
  // ui::Theme::instance().setPrimaryColor(QColor("#9b44f9"), QColor("#ffffff"));
  // ui::Theme::instance().setFontFamily("Inter, Segoe UI, sans-serif");

  // Optional: style global QToolTip popups to match current theme
  ui::Tooltip::applyGlobalStyle();
  QObject::connect(&ui::Theme::instance(), &ui::Theme::themeChanged, []() {
    ui::Tooltip::applyGlobalStyle();
  });

  // Create and show main window...
  return app.exec();
}
```

### Mandatory Reactive Widget Pattern for Host Pages/Widgets
Every custom page or composite widget in the host application **MUST** subscribe to `ui::Theme::themeChanged` and recompute its styles using semantic tokens:

```cpp
#include "theme/theme.h"
#include "theme/style_helper.h"

MyCustomPage::MyCustomPage(QWidget *parent) : QWidget(parent) {
  setObjectName("myCustomPage");
  setAttribute(Qt::WA_StyledBackground, true);

  setupUi();

  // 1. Connect to themeChanged signal
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged,
          this, &MyCustomPage::applyThemeStyles);

  // 2. Apply initial theme styles immediately
  applyThemeStyles();
}

void MyCustomPage::applyThemeStyles() {
  // 3. Read semantic token structs
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &s = ui::Theme::instance().spacing();
  const auto &t = ui::Theme::instance().typography();

  // 4. Apply fonts via TypographyTokens (t.font(...) or t.fontRem(...))
  m_titleLabel->setFont(t.font(t.sizeLg, QFont::Bold));
  m_subtitleLabel->setFont(t.fontRem(0.875, QFont::Normal));

  // 5. Build scoped QSS using StyleHelper::toHexString / toRgbaString
  setStyleSheet(QString(
      "QWidget#myCustomPage {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: %3px;"
      "}")
      .arg(ui::StyleHelper::toHexString(c.background))
      .arg(ui::StyleHelper::toHexString(c.border))
      .arg(r.md));

  m_titleLabel->setStyleSheet(
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground)));
}
```

> **STRICT RULE**: Never hardcode hex colors (`#ffffff`, `#18181b`) or raw pixel font sizes in custom widgets. Always query `ui::Theme::instance().colors()`, `radius()`, `spacing()`, and `typography()`.

---

## 5. Critical Qt / QSS Pitfalls & Rules for AI Agents

### Rule A: NEVER Use Unscoped `QFrame` Selectors in Stylesheets
In Qt's class hierarchy, **`QLabel` inherits from `QFrame`** (`class QLabel : public QFrame`).
- **WRONG**:
  ```cpp
  // BUG: Every child QLabel inside this widget will inherit a 1px border around its text!
  myContainer->setStyleSheet("QFrame { background: #18181b; border: 1px solid #27272a; }");
  ```
- **CORRECT**:
  Always scope QSS rules to the exact class selector (`ui--Card`, `ui--Toast`, `ui--Alert`) or a unique `objectName` (`QFrame#myPanel`, `QWidget#myContainer`) and enable `Qt::WA_StyledBackground`:
  ```cpp
  myContainer->setObjectName("myPanel");
  myContainer->setAttribute(Qt::WA_StyledBackground, true);
  myContainer->setStyleSheet("QWidget#myPanel { background: ...; border: 1px solid ...; }");
  ```

### Rule B: Qt Parent-Child Memory Ownership
- Always pass a valid `QWidget *parent` pointer when constructing `ui::*` components (e.g., `new ui::Button("Save", parent)`).
- **Exception / Self-Managed Lifecycle**: `ui::Toast::show(parent, title, description, variant, durationMs)` creates a floating overlay anchored to `parent`'s window, animates its opacity, and calls `deleteLater()` automatically upon dismissal. Do not manually `delete` the pointer returned by `ui::Toast::show`.

### Rule C: Signal Blocker Discipline for Synchronized Controls
When synchronizing two interactive widgets (such as a `ui::Slider` and a `ui::NumberInput`), always guard programmatic updates with `QSignalBlocker` to prevent infinite signal-slot recursion:
```cpp
connect(slider, &QSlider::valueChanged, this, [spinBox](int val) {
  const QSignalBlocker blocker(spinBox);
  spinBox->setValue(val);
});
connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [slider](int val) {
  const QSignalBlocker blocker(slider);
  slider->setValue(val);
});
```

### Rule D: Use Built-In `ui::Button` Loading State
Never manually swap button text, disable the button, and attach an external spinner when running an async/background task. Use `ui::Button`'s built-in loading API, which preserves the button's original text, icon, explicit disabled state, and layout `sizeHint`:
```cpp
saveBtn->setLoadingText("Converting...");
saveBtn->setLoading(true);   // Disables clicks, renders animated Spinner, prevents width collapse
// When finished:
saveBtn->setLoading(false);  // Restores original text, icon, and enabled state
```

---

## 6. Complete 30-Component API Reference & Selection Matrix

### Component Selection Matrix

| Category | Component | Header | Base Class | When to Use |
| :--- | :--- | :--- | :--- | :--- |
| **Buttons & Actions** | `ui::Button` | `components/button.h` | `QPushButton` | Primary CTAs, secondary actions, icon buttons, async loading triggers |
| **Buttons & Actions** | `ui::Menu` / `DropdownMenu` / `ContextMenu` | `components/menu.h` | `QMenu` | Action menus attached to buttons or right-click context menus |
| **Buttons & Actions** | `ui::Popover` | `components/popover.h` | `QFrame` | Anchored floating panel containing rich interactive widgets |
| **Forms & Inputs** | `ui::Input` | `components/input.h` | `QWidget` | Single-line text entry with optional prefix, suffix, or clear button |
| **Forms & Inputs** | `ui::Textarea` | `components/textarea.h` | `QPlainTextEdit` | Multi-line plain text notes or descriptions |
| **Forms & Inputs** | `ui::NumberInput` | `components/number_input.h` | `QSpinBox` | Integer numeric entry with themed step arrows and error state |
| **Forms & Inputs** | `ui::DecimalInput` | `components/number_input.h` | `QDoubleSpinBox` | Floating-point numeric entry with precision, suffix, and error state |
| **Forms & Inputs** | `ui::Select` | `components/select.h` | `QComboBox` | Fixed-list dropdown selection (non-searchable) |
| **Forms & Inputs** | `ui::Combobox` | `components/combobox.h` | `QWidget` | Searchable dropdown selection with type-to-filter popup |
| **Forms & Inputs** | `ui::Checkbox` | `components/checkbox.h` | `QCheckBox` | Boolean or tri-state check option with error state support |
| **Forms & Inputs** | `ui::RadioGroup` | `components/radio_group.h` | `QWidget` | Mutually exclusive list of radio options |
| **Forms & Inputs** | `ui::Switch` | `components/switch.h` | `QAbstractButton` | Instant boolean toggle switch with smooth thumb animation |
| **Forms & Inputs** | `ui::Slider` | `components/slider.h` | `QSlider` | Continuous or stepped range value selection |
| **Forms & Inputs** | `ui::FormField` | `components/form_field.h` | `QWidget` | Wraps any input widget with a label, `*` required mark, help text, and error message |
| **Feedback & Status** | `ui::Alert` | `components/alert.h` | `QFrame` | Persistent inline status message (`Info`, `Success`, `Warning`, `Destructive`) |
| **Feedback & Status** | `ui::Badge` | `components/badge.h` | `QLabel` | Compact pill badge for status, tags, or counts |
| **Feedback & Status** | `ui::Toast` | `components/toast.h` | `QFrame` | Transient auto-dismissing corner notification |
| **Feedback & Status** | `ui::Dialog` | `components/dialog.h` | `QDialog` | Modal confirmation, alert, or custom form dialog |
| **Feedback & Status** | `ui::Progress` | `components/progress.h` | `QWidget` | Determinate (`0..100`) or indeterminate animated progress bar |
| **Feedback & Status** | `ui::Spinner` | `components/spinner.h` | `QWidget` | Standalone rotating arc indicator for loading states |
| **Feedback & Status** | `ui::Tooltip` | `components/tooltip.h` | Utility class | Attaches tooltips and styles global `QToolTip` popups |
| **Content & Data** | `ui::Card` | `components/card.h` | `QFrame` | Grouped content box with title, description, content layout, and footer layout |
| **Content & Data** | `ui::Accordion` | `components/accordion.h` | `QWidget` | Expandable/collapsible disclosure section |
| **Content & Data** | `ui::Avatar` | `components/avatar.h` | `QWidget` | Circular avatar with initials or `QPixmap` and presence status dot |
| **Content & Data** | `ui::Table` | `components/table.h` | `QTableWidget` | Tabular data grid with themed headers and row selection |
| **Content & Data** | `ui::EmptyState` | `components/empty_state.h` | `QFrame` | Zero-data placeholder with icon, explanation, and optional action button |
| **Navigation & Layout** | `ui::Tabs` | `components/tabs.h` | `QWidget` | Multi-page view switcher (`Underline` or `Segmented` bar + `QStackedWidget`) |
| **Navigation & Layout** | `ui::Sidebar` | `components/sidebar.h` | `QFrame` | Collapsible left navigation rail with icons, labels, and footer slot |
| **Navigation & Layout** | `ui::ScrollArea` | `components/scroll_area.h` | `QScrollArea` | Scrollable viewport with themed thin scrollbars |
| **Navigation & Layout** | `ui::Splitter` | `components/splitter.h` | `QSplitter` | Draggable/keyboard-resizable split pane container |
| **Navigation & Layout** | `ui::Separator` | `components/separator.h` | `QFrame` | 1px horizontal or vertical visual divider |
| **Showcase** | `ui::DemoPage` | `demo/demo_page.h` | `QWidget` | Embeddable interactive gallery demonstrating all 30 components |

---

### Exact Constructor & Method Signatures

#### 1. `ui::Accordion` (`components/accordion.h`)
- **Constructors**:
  - `explicit Accordion(QWidget *parent = nullptr)`
  - `explicit Accordion(const QString &title, QWidget *content = nullptr, QWidget *parent = nullptr)`
- **Methods**: `title()`, `setTitle(const QString &)`, `isExpanded()`, `setExpanded(bool expanded, bool animated = true)`, `toggleExpanded()`, `setContentWidget(QWidget *)`, `contentWidget()`
- **Signals**: `void toggled(bool expanded)`

#### 2. `ui::Alert` (`components/alert.h`)
- **Enum**: `ui::AlertVariant { Info, Success, Warning, Destructive }`
- **Constructors**:
  - `explicit Alert(QWidget *parent = nullptr)`
  - `Alert(const QString &title, const QString &description, AlertVariant variant = AlertVariant::Info, QWidget *parent = nullptr)`
- **Methods**: `setTitle(const QString &)`, `title()`, `setDescription(const QString &)`, `description()`, `setVariant(AlertVariant)`, `variant()`

#### 3. `ui::Avatar` (`components/avatar.h`)
- **Enums**:
  - `ui::AvatarSize { Small, Default, Large }` (`28px`, `40px`, `56px`)
  - `ui::AvatarStatus { None, Online, Offline, Busy, Away }`
- **Constructors**:
  - `explicit Avatar(QWidget *parent = nullptr)`
  - `explicit Avatar(const QString &initials, AvatarSize size = AvatarSize::Default, QWidget *parent = nullptr)`
  - `explicit Avatar(const QPixmap &image, AvatarSize size = AvatarSize::Default, QWidget *parent = nullptr)`
- **Methods**: `setImage(const QPixmap &)`, `setInitials(const QString &)`, `size()`, `setSize(AvatarSize)`, `status()`, `setStatus(AvatarStatus)`

#### 4. `ui::Badge` (`components/badge.h`)
- **Enum**: `ui::BadgeVariant { Default, Secondary, Outline, Destructive }`
- **Constructors**:
  - `explicit Badge(QWidget *parent = nullptr)`
  - `explicit Badge(const QString &text, QWidget *parent = nullptr)`
  - `Badge(const QString &text, BadgeVariant variant, QWidget *parent = nullptr)`
- **Methods**: `variant()`, `setVariant(BadgeVariant)`

#### 5. `ui::Button` (`components/button.h`)
- **Enums**:
  - `ui::ButtonVariant { Primary, Secondary, Outline, Ghost, Destructive, Link }`
  - `ui::ButtonSize { Small, Default, Large, Icon }`
- **Constructors**:
  - `explicit Button(QWidget *parent = nullptr)`
  - `explicit Button(const QString &text, QWidget *parent = nullptr)`
  - `Button(const QString &text, ButtonVariant variant, ButtonSize size = ButtonSize::Default, QWidget *parent = nullptr)`
- **Methods**: `variant()`, `setVariant(ButtonVariant)`, `size()`, `setSize(ButtonSize)`, `isLoading()`, `setLoading(bool)`, `loadingText()`, `setLoadingText(const QString &)`

#### 6. `ui::Card` (`components/card.h`)
- **Constructors**:
  - `explicit Card(QWidget *parent = nullptr)`
  - `Card(const QString &title, const QString &description = "", QWidget *parent = nullptr)`
- **Methods**: `setTitle(const QString &)`, `setDescription(const QString &)`, `setContentWidget(QWidget *)`, `setContentLayout(QLayout *)`, `setFooterWidget(QWidget *)`, `setFooterLayout(QLayout *)`, `contentLayout()` (`QVBoxLayout *`), `footerLayout()` (`QHBoxLayout *`)

#### 7. `ui::Checkbox` (`components/checkbox.h`)
- **Constructors**:
  - `explicit Checkbox(QWidget *parent = nullptr)`
  - `explicit Checkbox(const QString &text, QWidget *parent = nullptr)`
- **Methods**: `hasError()`, `setError(bool)` (plus all `QCheckBox` methods: `setChecked`, `setTristate`, `setCheckState`, etc.)

#### 8. `ui::Combobox` (`components/combobox.h`)
- **Constructors**:
  - `explicit Combobox(QWidget *parent = nullptr)`
  - `explicit Combobox(const QStringList &items, QWidget *parent = nullptr)`
- **Methods**: `setItems(const QStringList &)`, `addItem(const QString &text, const QVariant &data = QVariant())`, `currentIndex()`, `setCurrentIndex(int)`, `currentText()`, `currentData()`, `setPlaceholderText(const QString &)`, `hasError()`, `setError(bool)`
- **Signals**: `void currentIndexChanged(int index)`, `void currentTextChanged(const QString &text)`

#### 9. `ui::Dialog` (`components/dialog.h`)
- **Constructors**:
  - `explicit Dialog(QWidget *parent = nullptr)`
  - `Dialog(const QString &title, const QString &description = "", QWidget *parent = nullptr)`
- **Instance Methods**: `setTitle(const QString &)`, `setDescription(const QString &)`, `setContentWidget(QWidget *)`, `setContentLayout(QLayout *)`, `setConfirmText(const QString &)`, `setCancelText(const QString &)`, `setDestructive(bool)`, `confirmButton()`, `cancelButton()`
- **Static Convenience Methods**:
  - `static bool confirm(QWidget *parent, const QString &title, const QString &description, const QString &confirmText = "Confirm", bool isDestructive = false)`
  - `static void info(QWidget *parent, const QString &title, const QString &description)`

#### 10. `ui::EmptyState` (`components/empty_state.h`)
- **Constructors**:
  - `explicit EmptyState(QWidget *parent = nullptr)`
  - `EmptyState(const QString &title, const QString &description, QWidget *parent = nullptr)`
- **Methods**: `setTitle(const QString &)`, `title()`, `setDescription(const QString &)`, `description()`, `setIcon(const QIcon &)`, `setActionText(const QString &)`, `actionButton()`
- **Signals**: `void actionTriggered()`

#### 11. `ui::FormField` (`components/form_field.h`)
- **Constructors**:
  - `explicit FormField(QWidget *parent = nullptr)`
  - `explicit FormField(const QString &label, QWidget *control = nullptr, QWidget *parent = nullptr)`
- **Methods**: `label()`, `setLabel(const QString &)`, `isRequired()`, `setRequired(bool)`, `setDescription(const QString &)`, `description()`, `setError(const QString &errorMessage)`, `clearError()`, `hasError()`, `error()`, `setControl(QWidget *)`, `control()`

#### 12. `ui::Input` (`components/input.h`)
- **Constructors**:
  - `explicit Input(QWidget *parent = nullptr)`
  - `explicit Input(const QString &placeholder, QWidget *parent = nullptr)`
- **Methods**: `text()`, `setText(const QString &)`, `placeholderText()`, `setPlaceholderText(const QString &)`, `setPrefixText(const QString &)`, `setSuffixText(const QString &)`, `isClearButtonEnabled()`, `setClearButtonEnabled(bool)`, `isReadOnly()`, `setReadOnly(bool)`, `hasError()`, `setError(bool)`, `lineEdit()`
- **Signals**: `void textChanged(const QString &)`, `void textEdited(const QString &)`, `void returnPressed()`

#### 13. `ui::Menu` / `ui::DropdownMenu` / `ui::ContextMenu` (`components/menu.h`)
- **Constructors**:
  - `explicit Menu(QWidget *parent = nullptr)`
  - `explicit Menu(const QString &title, QWidget *parent = nullptr)`
- **Type Aliases**: `using DropdownMenu = Menu;`, `using ContextMenu = Menu;` (inherits all `QMenu` methods: `addAction`, `addSeparator`, `addMenu`, `popup`, etc.)

#### 14. `ui::NumberInput` & `ui::DecimalInput` (`components/number_input.h`)
- **Constructors**:
  - `explicit NumberInput(QWidget *parent = nullptr)` (inherits `QSpinBox`)
  - `explicit DecimalInput(QWidget *parent = nullptr)` (inherits `QDoubleSpinBox`)
- **Methods**: `hasError()`, `setError(bool)` (plus `setRange`, `setValue`, `setSingleStep`, `setSuffix`, `setPrefix`, `setDecimals`, `setReadOnly`)

#### 15. `ui::Popover` (`components/popover.h`)
- **Constructor**: `explicit Popover(QWidget *parent = nullptr)`
- **Methods**: `contentLayout()` (`QVBoxLayout *`), `openFor(QWidget *anchor)`, `closePopover()`
- **Signals**: `void opened()`, `void closed()`

#### 16. `ui::Progress` (`components/progress.h`)
- **Enum**: `ui::ProgressSize { Small, Default, Large }` (`4px`, `8px`, `12px`)
- **Constructors**:
  - `explicit Progress(QWidget *parent = nullptr)`
  - `explicit Progress(int value, QWidget *parent = nullptr)`
- **Methods**: `value()`, `setValue(int value, bool animated = true)`, `minimum()`, `setMinimum(int)`, `maximum()`, `setMaximum(int)`, `setRange(int min, int max)`, `isIndeterminate()`, `setIndeterminate(bool)`, `size()`, `setSize(ProgressSize)`

#### 17. `ui::RadioGroup` (`components/radio_group.h`)
- **Constructors**:
  - `explicit RadioGroup(QWidget *parent = nullptr)`
  - `explicit RadioGroup(const QStringList &options, QWidget *parent = nullptr)`
- **Methods**: `addOption(const QString &text, int id = -1)`, `checkedId()`, `setCheckedId(int id)`, `button(int id)` (`QRadioButton *`), `orientation()`, `setOrientation(Qt::Orientation)`
- **Signals**: `void selectionChanged(int id)`

#### 18. `ui::ScrollArea` (`components/scroll_area.h`)
- **Constructor**: `explicit ScrollArea(QWidget *parent = nullptr)` (inherits `QScrollArea`)

#### 19. `ui::Select` (`components/select.h`)
- **Constructors**:
  - `explicit Select(QWidget *parent = nullptr)`
  - `explicit Select(const QStringList &items, QWidget *parent = nullptr)` (inherits `QComboBox`)

#### 20. `ui::Separator` (`components/separator.h`)
- **Constructor**: `explicit Separator(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr)`
- **Methods**: `orientation()`, `setOrientation(Qt::Orientation)`

#### 21. `ui::Sidebar` (`components/sidebar.h`)
- **Constructor**: `explicit Sidebar(QWidget *parent = nullptr)`
- **Methods**: `isCollapsed()`, `setCollapsed(bool collapsed, bool animated = true)`, `toggleCollapsed()`, `setTitle(const QString &title, const QIcon &icon = QIcon())`, `addItem(const QString &text, const QIcon &icon = QIcon(), int id = -1)`, `setCurrentIndex(int)`, `currentIndex()`, `sidebarWidth()`, `setSidebarWidth(int)`, `setFooterWidget(QWidget *)`
- **Signals**: `void itemSelected(int id, int index)`, `void collapsedChanged(bool isCollapsed)`

#### 22. `ui::Slider` (`components/slider.h`)
- **Constructors**:
  - `explicit Slider(QWidget *parent = nullptr)`
  - `explicit Slider(Qt::Orientation orientation, QWidget *parent = nullptr)` (inherits `QSlider`)

#### 23. `ui::Spinner` (`components/spinner.h`)
- **Constructors**:
  - `explicit Spinner(QWidget *parent = nullptr)`
  - `explicit Spinner(int size, QWidget *parent = nullptr)`
- **Methods**: `size()`, `setSpinnerSize(int)`, `color()`, `setColor(const QColor &)`, `start()`, `stop()`, `isSpinning()`

#### 24. `ui::Splitter` (`components/splitter.h`)
- **Constructors**:
  - `explicit Splitter(QWidget *parent = nullptr)`
  - `explicit Splitter(Qt::Orientation orientation, QWidget *parent = nullptr)` (inherits `QSplitter`)

#### 25. `ui::Switch` (`components/switch.h`)
- **Constructors**:
  - `explicit Switch(QWidget *parent = nullptr)`
  - `explicit Switch(bool checked, QWidget *parent = nullptr)` (inherits `QAbstractButton`; use `isChecked()`, `setChecked(bool)`, `toggled(bool)`)

#### 26. `ui::Table` (`components/table.h`)
- **Constructors**:
  - `explicit Table(QWidget *parent = nullptr)`
  - `Table(int rows, int columns, QWidget *parent = nullptr)` (inherits `QTableWidget`)

#### 27. `ui::Tabs` (`components/tabs.h`)
- **Enum**: `ui::TabVariant { Underline, Segmented }`
- **Constructors**:
  - `explicit Tabs(QWidget *parent = nullptr)`
  - `explicit Tabs(TabVariant variant, QWidget *parent = nullptr)`
- **Methods**: `variant()`, `setVariant(TabVariant)`, `addTab(const QString &text, const QIcon &icon = QIcon(), QWidget *page = nullptr)`, `addTab(const QString &text, const QString &iconPath, QWidget *page = nullptr)`, `insertTab(...)`, `removeTab(int)`, `currentIndex()`, `setCurrentIndex(int)`, `count()`, `tabText(int)`, `setTabText(int, const QString &)`, `widget(int)`
- **Signals**: `void currentChanged(int index)`

#### 28. `ui::Textarea` (`components/textarea.h`)
- **Constructors**:
  - `explicit Textarea(QWidget *parent = nullptr)`
  - `explicit Textarea(const QString &placeholder, QWidget *parent = nullptr)` (inherits `QPlainTextEdit`)
- **Methods**: `hasError()`, `setError(bool)`

#### 29. `ui::Toast` (`components/toast.h`)
- **Enum**: `ui::ToastVariant { Default, Success, Warning, Destructive }`
- **Static Helper (Recommended)**:
  - `static Toast *show(QWidget *parent, const QString &title, const QString &description = "", ToastVariant variant = ToastVariant::Default, int durationMs = 3000)`
- **Instance Methods**: `setTitle(const QString &)`, `setDescription(const QString &)`, `setVariant(ToastVariant)`, `setDuration(int)`, `showToast()`, `dismiss()`

#### 30. `ui::Tooltip` (`components/tooltip.h`)
- **Static Methods**:
  - `static void attach(QWidget *widget, const QString &tooltipText)`
  - `static void applyGlobalStyle()`

#### Showcase: `ui::DemoPage` (`demo/demo_page.h`)
- **Constructor**: `explicit DemoPage(QWidget *parent = nullptr)`
- **Purpose**: Self-contained scrollable gallery widget demonstrating all 30 `ui-kit` components with interactive states and live theme switching.
