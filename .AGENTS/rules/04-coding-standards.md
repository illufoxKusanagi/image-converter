# Coding Standards & Guidelines

## 1. C++ Standard & Idioms
- **Language Level**: C++17.
- **Includes**: Group standard library headers, Qt library headers, and local project headers with alphabetical sorting within groups.
- **Header Guards**: Use `#ifndef CLASSNAME_H` / `#define CLASSNAME_H` / `#endif`.
- **Formatting**: Adhere to `clang-format` rules with 2-space indentation and clean alignment.

---

## 2. Qt Memory Management Rules
- Always pass parent pointers `(QWidget *parent = nullptr)` to constructors of `QObject` / `QWidget` subclasses.
- Layouts instantiated with a parent `(new QVBoxLayout(this))` automatically take ownership of widgets added via `addWidget()` or `addLayout()`.
- Avoid naked `delete` calls on `QWidget` pointers unless disconnected from parent ownership.
- Dialogs displayed with `.exec()` (e.g. `MessageBoxWidget`) should be allocated on the stack where possible.

---

## 3. Signal & Slot Best Practices
- Prefer modern C++11 function pointer syntax:
  ```cpp
  connect(button, &QPushButton::clicked, this, &MainPage::onProcessButtonClicked);
  ```
- For overloaded signals (such as `QSpinBox::valueChanged(int)`), use `QOverload<int>::of(&QSpinBox::valueChanged)`.
- When updating bidirectional controls (e.g. `SliderWidget`), block signals during updates:
  ```cpp
  m_slider->blockSignals(true);
  m_slider->setValue(m_spinBox->value());
  m_slider->blockSignals(false);
  ```

---

## 4. File Paths & Cross-Platform Conventions
- Always normalize file paths using `QDir::cleanPath(QDir::fromNativeSeparators(path))`.
- When constructing paths, use `QDir(dirPath).filePath(fileName)` or `QFileInfo`.
- Never hardcode Windows-specific separators (`\`) in source strings.

---

## 5. UI Modularity & Component Conventions
- Encapsulate custom styling within the widget class implementation.
- All styles must consume `Colors::*` and `TextStyle::*` to preserve unified branding.
- Avoid placing direct file processing logic inside generic UI widgets (except for self-contained helper routines like `DropFileWidget::saveImage()`).
