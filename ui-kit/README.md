# Qt UI-Kit (`ui-kit`)

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Qt 6 / Qt 5](https://img.shields.io/badge/Qt-6%20%7C%205-41CD52.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)
![Shadcn-Inspired](https://img.shields.io/badge/Design-Shadcn%20UI-9b44f9.svg)

A modern, zero-dependency **Shadcn-inspired component library for Qt 6 and Qt 5 Widgets** written in clean C++17.

`ui-kit` brings the crisp visual hierarchy, semantic token architecture, and smooth micro-interactions of modern web design systems to native C++ desktop applications—without QML overhead or third-party dependencies.

> **Using an AI coding assistant?** Point your agent directly at [**`AI_AGENT_GUIDE.md`**](AI_AGENT_GUIDE.md) for machine-actionable CMake instructions, QSS scoping rules, and a complete 30-component API contract.

---

## Key Features

- **Zero External Non-Qt Dependencies**: Built strictly on `Qt::Core`, `Qt::Widgets`, and `Qt::Svg`.
- **Reactive Light, Dark & System Themes**: Instant runtime theme switching via `ui::Theme::instance()` with automatic OS dark-mode detection (`ThemeMode::System`).
- **Tokenized Design System**: Semantic `ColorTokens`, `RadiusTokens`, `SpacingTokens`, and `TypographyTokens` (including `rem`-based font scaling via `fontRem(double)`).
- **30 Self-Contained Components**: Buttons, forms, searchable comboboxes, popovers, menus, dialogs, toasts, tables, collapsible sidebars, splitters, and more.
- **Keyboard & Screen-Reader Accessible**: Native focus rings, buddy labels, `accessibleName`/`accessibleDescription` propagation, and keyboard navigation (`Enter`, `Escape`, arrow keys, `Shift+F10`).
- **Built-In Interactive Showcase (`ui::DemoPage`)**: Embed `ui::DemoPage` in any window or build the standalone `ui_kit_demo` executable to inspect every component live.

---

## Quick Start

### 1. Add to CMake (`CMakeLists.txt`)

#### Using `add_subdirectory` (Submodule or Vendor Folder)
```cmake
cmake_minimum_required(VERSION 3.16)
project(MyDesktopApp LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg)

add_subdirectory(ui-kit)

add_executable(MyDesktopApp
    main.cpp
    resources/icons.qrc
)

target_link_libraries(MyDesktopApp PRIVATE ui_kit)
```

#### Using CMake `FetchContent`
```cmake
include(FetchContent)
FetchContent_Declare(
    ui_kit
    GIT_REPOSITORY https://github.com/your-org/ui-kit.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(ui_kit)

target_link_libraries(MyDesktopApp PRIVATE ui_kit)
```

### 2. Minimal `main.cpp` Example
```cpp
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include "theme/theme.h"
#include "components/card.h"
#include "components/input.h"
#include "components/form_field.h"
#include "components/button.h"
#include "components/toast.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Follow OS light/dark preference (or ThemeMode::Light / ThemeMode::Dark)
  ui::Theme::instance().setMode(ui::ThemeMode::System);

  QMainWindow window;
  window.setWindowTitle("Qt UI-Kit Quick Start");
  window.resize(480, 320);

  auto *card = new ui::Card("Project Settings",
                            "Configure your workspace preferences", &window);

  auto *nameInput = new ui::Input("my-awesome-app", card);
  nameInput->setClearButtonEnabled(true);
  auto *field = new ui::FormField("Workspace Name", nameInput, card);
  field->setRequired(true);
  card->contentLayout()->addWidget(field);

  auto *saveBtn = new ui::Button("Save Changes", ui::ButtonVariant::Primary,
                                 ui::ButtonSize::Default, card);
  QObject::connect(saveBtn, &QPushButton::clicked, &window, [&window]() {
    ui::Toast::show(&window, "Saved", "Workspace preferences updated.",
                    ui::ToastVariant::Success);
  });
  card->footerLayout()->addStretch();
  card->footerLayout()->addWidget(saveBtn);

  window.setCentralWidget(card);
  window.show();
  return app.exec();
}
```

---

## Theme & Token System

All visual properties are governed by `ui::Theme::instance()` (`theme/theme.h` & `theme/tokens.h`):

| Token Struct / Helper | Header | Purpose |
| :--- | :--- | :--- |
| `ui::Theme::instance()` | `theme/theme.h` | Singleton managing `ThemeMode` (`Light`, `Dark`, `System`), emitting `themeChanged(ui::ThemeMode)` |
| `ui::ColorTokens` | `theme/tokens.h` | Semantic surfaces & foregrounds (`background`, `foreground`, `card`, `popover`, `primary`, `secondary`, `muted`, `accent`, `destructive`, `border`, `input`, `ring`, `success`, `warning`, `info`) |
| `ui::RadiusTokens` | `theme/tokens.h` | Corner radii (`none=0`, `sm=4`, `md=6`, `lg=8`, `xl=12`, `full=9999`) |
| `ui::SpacingTokens` | `theme/tokens.h` | 4px-grid spacing scale (`space0=0` through `space12=48`) |
| `ui::TypographyTokens` | `theme/tokens.h` | Font stack, pixel sizes (`sizeXs`..`size3Xl`), `font(px, weight)`, and `fontRem(rem, weight)` |
| `ui::StyleHelper` | `theme/style_helper.h` | Color formatting & manipulation (`toHexString`, `toRgbaString`, `withAlpha`, `lighten`, `darken`, `hoverColor`, `tint`) |
| `ui::AnimationHelper` | `theme/animation_helper.h` | Standard durations (`DurationFast=150`, `DurationNormal=200`, `DurationSlow=300`) & easing curves |
| `ui::Icon` | `theme/icon_helper.h` | Runtime SVG tinting (`ui::Icon::get(resourcePath, color, size)`) |

### Customizing Brand Colors & Typography
```cpp
auto &theme = ui::Theme::instance();
theme.setPrimaryColor(QColor("#2563eb"), QColor("#ffffff"));
theme.setFontFamily("Inter, Segoe UI, Roboto, sans-serif");
theme.toggleMode(); // Cycle between Light and Dark mode
```

---

## Component Catalog (All 30 Components)

### 1. Buttons & Actions (`Button`, `Menu`, `Popover`)
```cpp
#include "components/button.h"
#include "components/menu.h"
#include "components/popover.h"

// Button with variants (Primary, Secondary, Outline, Ghost, Destructive, Link) & loading state
auto *btn = new ui::Button("Export", ui::ButtonVariant::Primary, ui::ButtonSize::Default, parent);
btn->setLoadingText("Exporting...");
btn->setLoading(true);

// DropdownMenu / ContextMenu
auto *menu = new ui::DropdownMenu(btn);
menu->addAction("Export as PNG");
menu->addSeparator();
menu->addAction("Reset Defaults");
btn->setMenu(menu);

// Anchored Popover
auto *popover = new ui::Popover(btn);
popover->contentLayout()->addWidget(new ui::Input("Filter value...", popover));
popover->openFor(btn);
```

### 2. Text & Numeric Inputs (`Input`, `Textarea`, `NumberInput`, `DecimalInput`, `FormField`)
```cpp
#include "components/input.h"
#include "components/textarea.h"
#include "components/number_input.h"
#include "components/form_field.h"

auto *input = new ui::Input("Enter filename...", parent);
input->setPrefixText("img_");
input->setSuffixText(".webp");
input->setClearButtonEnabled(true);

auto *field = new ui::FormField("Output Filename", input, parent);
field->setRequired(true);
field->setDescription("Used for batch exports");

auto *notes = new ui::Textarea("Add export metadata notes...", parent);

auto *quality = new ui::NumberInput(parent);
quality->setRange(1, 100);
quality->setSuffix(" %");
quality->setValue(85);

auto *scale = new ui::DecimalInput(parent);
scale->setDecimals(2);
scale->setRange(0.25, 4.0);
scale->setSingleStep(0.25);
scale->setSuffix("x");
scale->setValue(1.50);
```

### 3. Selection Controls (`Select`, `Combobox`, `Checkbox`, `RadioGroup`, `Switch`, `Slider`)
```cpp
#include "components/select.h"
#include "components/combobox.h"
#include "components/checkbox.h"
#include "components/radio_group.h"
#include "components/switch.h"
#include "components/slider.h"

// Fixed dropdown
auto *select = new ui::Select({"PNG", "JPEG", "WEBP", "TIFF"}, parent);

// Searchable filter combobox
auto *combo = new ui::Combobox(parent);
combo->addItem("PNG (Lossless)", "png");
combo->addItem("WEBP (Modern Web)", "webp");
combo->setPlaceholderText("Search formats...");

// Checkbox & RadioGroup
auto *check = new ui::Checkbox("Preserve EXIF metadata", parent);
auto *radios = new ui::RadioGroup({"Standard", "Balanced", "Maximum"}, parent);
radios->setCheckedId(1);

// Animated Switch & Slider
auto *toggle = new ui::Switch(true, parent);
auto *slider = new ui::Slider(Qt::Horizontal, parent);
slider->setRange(0, 100);
slider->setValue(75);
```

### 4. Feedback & Overlays (`Alert`, `Badge`, `Toast`, `Dialog`, `Progress`, `Spinner`, `Tooltip`)
```cpp
#include "components/alert.h"
#include "components/badge.h"
#include "components/toast.h"
#include "components/dialog.h"
#include "components/progress.h"
#include "components/spinner.h"
#include "components/tooltip.h"

// Persistent inline alert (Info, Success, Warning, Destructive)
auto *alert = new ui::Alert("Batch Ready", "12 images queued for conversion.",
                            ui::AlertVariant::Info, parent);

// Status Badge
auto *badge = new ui::Badge("Lossless", ui::BadgeVariant::Secondary, parent);

// Floating Toast notification
ui::Toast::show(parent, "Conversion Complete", "Saved 12 files to disk.",
                ui::ToastVariant::Success);

// Modal Dialog (Confirm / Info)
if (ui::Dialog::confirm(parent, "Overwrite Files?",
                        "Existing files in the output folder will be replaced.",
                        "Overwrite", /*isDestructive=*/true)) {
  // Proceed...
}

// Progress & Spinner
auto *progress = new ui::Progress(65, parent);
auto *spinner = new ui::Spinner(24, parent);
spinner->start();

// Tooltip
ui::Tooltip::attach(badge, "PNG compression preserves 100% pixel fidelity");
```

### 5. Containers & Data Display (`Card`, `Accordion`, `Avatar`, `Table`, `EmptyState`)
```cpp
#include "components/card.h"
#include "components/accordion.h"
#include "components/avatar.h"
#include "components/table.h"
#include "components/empty_state.h"

// Card container
auto *card = new ui::Card("Compression Summary", "Recent batch statistics", parent);

// Collapsible Accordion
auto *accordion = new ui::Accordion("Advanced Color Quantization", detailsWidget, parent);

// User/Entity Avatar
auto *avatar = new ui::Avatar("IK", ui::AvatarSize::Default, parent);
avatar->setStatus(ui::AvatarStatus::Online);

// Themed Data Table
auto *table = new ui::Table(3, 3, parent);
table->setHorizontalHeaderLabels({"File", "Format", "Status"});

// EmptyState placeholder
auto *empty = new ui::EmptyState("No Files Queued",
                                 "Drag and drop images here to begin.", parent);
empty->setActionText("Browse Files");
```

### 6. Navigation & Layout (`Tabs`, `Sidebar`, `ScrollArea`, `Splitter`, `Separator`)
```cpp
#include "components/tabs.h"
#include "components/sidebar.h"
#include "components/scroll_area.h"
#include "components/splitter.h"
#include "components/separator.h"

// Tabs (TabVariant::Underline or TabVariant::Segmented)
auto *tabs = new ui::Tabs(ui::TabVariant::Underline, parent);
tabs->addTab("Image Converter", QIcon(), imagePage);
tabs->addTab("PDF Compression", QIcon(), pdfPage);

// Collapsible Sidebar
auto *sidebar = new ui::Sidebar(parent);
sidebar->setTitle("Workspace");
sidebar->addItem("Convert", QIcon(), 0);
sidebar->addItem("Showcase", QIcon(), 1);

// ScrollArea, Splitter & Separator
auto *scroll = new ui::ScrollArea(parent);
auto *splitter = new ui::Splitter(Qt::Horizontal, parent);
auto *divider = new ui::Separator(Qt::Horizontal, parent);
```

---

## Building & Running the Standalone Demo Showcase

`ui-kit` includes a standalone demo application (`ui_kit_demo`) powered by `ui::DemoPage` (`demo/demo_page.h`), showcasing all 30 components with live Light/Dark/System theme switching.

### Build from the `ui-kit/` Directory (Standalone)
```bash
cmake -B build -G Ninja -DUI_KIT_BUILD_DEMO=ON
cmake --build build --parallel
./build/ui_kit_demo
```

### Build from a Parent Project
```bash
cmake -B build -G Ninja -DUI_KIT_BUILD_DEMO=ON
cmake --build build --target ui_kit_demo --parallel
./build/ui-kit/ui_kit_demo
```

### Embedding `ui::DemoPage` Inside Your Own App
Because `demo/demo_page.cpp` is compiled directly into the `ui_kit` static library, you can drop the interactive showcase into any tab or window with two lines of code:
```cpp
#include "demo/demo_page.h"

tabs->addTab("Component Showcase", QIcon(), new ui::DemoPage(this));
```

---

## AI Agent Integration

See [**`AI_AGENT_GUIDE.md`**](AI_AGENT_GUIDE.md) for comprehensive architectural rules, required `.qrc` icon paths, QSS pitfalls to avoid (such as `QLabel` inheriting from `QFrame`), and complete method signatures.
