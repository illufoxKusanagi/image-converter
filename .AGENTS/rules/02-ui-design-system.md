# UI Design System & Component Guidelines

## 1. Design Token Architecture
All styling in ImageConverter is centralized in two headers:
1. [styles/colors.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/colors.h): Namespace `Colors` containing `QColor` constants.
2. [styles/text_style.h](file:///d:/matkul/sem_6/AppProject/ImageConverter/styles/text_style.h): Namespace `TextStyle` returning inline CSS snippet strings with `font-family: 'Roboto'`.

---

## 2. Color Palette Catalog

| Palette Group | Key Tokens | Usage |
| :--- | :--- | :--- |
| **Standard** | `Colors::StandardWhite`, `Colors::StandardBlack` | Base canvas background, high-contrast text |
| **Primary** (Purple) | `Primary50` (`#EBDAFE`) to `Primary900` (`#341753`) | Primary CTA buttons (`Primary600`), focus states (`Primary700`), hover states (`Primary400`), slider handles (`Primary500`) |
| **Secondary** (Blue) | `Secondary50` (`#D4E7FA`) to `Secondary900` (`#0D2C4D`) | Tab highlights (`Secondary500`), container borders (`Secondary400`), sidebar background (`Secondary600`) |
| **Grey** (Neutrals) | `Grey100` (`#E2E3E2`) to `Grey900` (`#161719`) | Disabled buttons (`Grey400`), slider groove (`Grey200`), text captions (`Grey700`, `Grey900`) |
| **Danger** (Red) | `Danger100` (`#FCDAD9`) to `Danger900` (`#300E0D`) | Error labels (`Danger600`), error borders |
| **Warning** (Yellow) | `Warning100` (`#FFEECC`) to `Warning900` (`#332200`) | Warnings & cautions |

---

## 3. Typography Rules

Font Family: **Roboto** (Google Font)

| Token Method | CSS Output Properties | Typical Usage |
| :--- | :--- | :--- |
| `TextStyle::Heading1()` | `font-size: 48px; font-weight: 600;` | Major titles |
| `TextStyle::Heading2()` | `font-size: 40px; font-weight: 600;` | Section headers |
| `TextStyle::Heading3()` | `font-size: 32px; font-weight: 600;` | Page headers |
| `TextStyle::Heading4()` | `font-size: 28px; font-weight: 600;` | Card headers |
| `TextStyle::Heading5()` | `font-size: 24px; font-weight: 600;` | Subheaders |
| `TextStyle::BodyBigBold()` | `font-size: 18px; font-weight: 600;` | Prominent button labels, dropzone headers |
| `TextStyle::BodyBigRegular()` | `font-size: 18px; font-weight: 400;` | Long body descriptions |
| `TextStyle::BodyMediumBold()` | `font-size: 16px; font-weight: 600;` | Active tabs, standard CTA button text |
| `TextStyle::BodyMediumRegular()` | `font-size: 16px; font-weight: 400;` | Inactive tabs, form labels, message box text |
| `TextStyle::BodySmallBold()` | `font-size: 14px; font-weight: 600;` | Badge text, separator text ("or") |
| `TextStyle::BodySmallRegular()` | `font-size: 14px; font-weight: 400;` | Input field text, spinbox text |
| `TextStyle::Subttile*()` | `font-size: 10px–14px;` | Metadata, helper descriptions, upload status |

---

## 4. Component Rules & Construction Guidelines

### 4.1 Buttons (`ButtonAction`)
- Always use `setSize(width, height)` or `setFixedSize()` for predictable layout boundaries.
- Active CTA buttons must have:
  - Background: `Colors::Primary600.name()`
  - Text color: `Colors::StandardWhite.name()`
  - Border radius: `12px`
  - Hover background: `Colors::Primary400.name()`
  - Pressed background: `Colors::Primary500.name()`
- Disabled state must switch background to `Colors::Grey400.name()`.

### 4.2 Drop Zones (`DropFileWidget`)
- Fixed dimensions: `320px x 320px`.
- Border: `2px solid Colors::Secondary400.name()`, radius `12px`.
- Dynamic colorization of SVGs: Use `createColoredIcon()` using `QPainter::CompositionMode_SourceIn` to tint icons directly with `QColor`.

### 4.3 Sliders & Inputs (`SliderWidget`, `InputDropdown`)
- Border radius on controls: `8px`.
- Controls must maintain high contrast against white backgrounds.
- Dropdown arrow and spinbox up/down arrows must be loaded from `:/icons/icons/*.svg`.

### 4.4 Modals (`MessageBoxWidget`)
- Use `MessageBoxWidget` instead of raw `QMessageBox` to maintain consistent typography and `Primary500` themed action buttons.
