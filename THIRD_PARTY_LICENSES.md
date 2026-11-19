# Third-Party Licenses & Notices

**ImageConverter** is licensed under the **GNU Affero General Public License, Version 3 (AGPL-3.0-or-later)** — see [`LICENSE`](LICENSE) for the full text.

ImageConverter uses and/or distributes the following third-party libraries, tools, and assets, each governed by its respective license terms below.

---

## 1. Qt 6 Framework

- **Project**: Qt 6 (`Qt6::Core`, `Qt6::Gui`, `Qt6::Widgets`, `Qt6::Pdf`, `Qt6::Concurrent`, `Qt6::Svg`)
- **Copyright**: Copyright © The Qt Company Ltd. and other contributors.
- **License**: **GNU Lesser General Public License, Version 3 (LGPL-3.0-only)** (also available under GPLv3 or commercial Qt licenses).
- **Project Website & Licensing**:
  - https://www.qt.io/
  - https://www.qt.io/licensing/
  - https://doc.qt.io/qt-6/licensing.html
- **LGPLv3 License Text**: https://www.gnu.org/licenses/lgpl-3.0.txt
- **Corresponding Qt Source Code**: https://www.qt.io/download-open-source / https://code.qt.io/

### LGPLv3 Compliance Notice
ImageConverter **dynamically links** against the Qt 6 libraries (`Core`, `Gui`, `Widgets`, `Pdf`, `Concurrent`, and `Svg`). In accordance with the terms of the GNU LGPLv3, you have the right to obtain the corresponding source code for Qt, modify those Qt libraries, and replace or relink the dynamically linked Qt shared libraries used by ImageConverter with your own modified versions.

### Bundled Third-Party Components in `Qt6::Pdf` (PDFium)
The `Qt6::Pdf` module bundles **PDFium** (maintained by The Chromium Authors / Google and originally derived from Foxit PDF SDK), which is licensed under the **BSD 3-Clause License**, along with third-party dependencies within PDFium and Qt licensed under permissive open-source licenses including **Apache-2.0**, **BSD-3-Clause**, **MIT**, **Zlib**, and **libpng** (such as FreeType, HarfBuzz, LittleCMS, libjpeg-turbo, OpenJPEG, and zlib).

For full attribution details and component-level notices included with Qt 6 and `QtPdf`:
- https://doc.qt.io/qt-6/qtpdf-attribution-chromium-pdfium.html
- https://doc.qt.io/qt-6/licenses-used-in-qt.html

---

## 2. Ghostscript

- **Project**: Ghostscript
- **Copyright**: Copyright © Artifex Software, Inc.
- **License**: **GNU Affero General Public License, Version 3 (AGPL-3.0-or-later)** (commercial licenses are also available from Artifex Software, Inc.).
- **Project Website**: https://www.ghostscript.com/
- **Source Repository**: https://github.com/ArtifexSoftware/ghostpdl
- **AGPLv3 License Text**: See [`LICENSE`](LICENSE) or https://www.gnu.org/licenses/agpl-3.0.txt

### Usage Notice
ImageConverter uses Ghostscript as an optional external CLI / packaged PDF processing engine (invoked as a subprocess via `gs` / `gswin64c`, and declared as a package dependency in Linux `.deb`/`.rpm` distributions) for advanced PDF compression and processing operations. ImageConverter is licensed under the **GNU AGPLv3**, maintaining full license compatibility with AGPLv3-licensed Ghostscript.

---

## 3. Lucide Icons

- **Project**: Lucide Icons (`resources/icons/lucide-*.svg`)
- **Website**: https://lucide.dev/
- **Repository**: https://github.com/lucide-icons/lucide
- **License**: **ISC License** (with portions derived from Feather Icons under the **MIT License**)

### Copyright & License Text

```text
ISC License

Copyright (c) for portions of Lucide are held by Cole Bemis 2013-2022 as part of Feather (MIT). All other copyright (c) for Lucide are held by Lucide Contributors 2022.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```

#### Feather Icons (Portions — MIT License)

```text
The MIT License (MIT)

Copyright (c) 2013-2022 Cole Bemis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
