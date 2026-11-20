#include "pages/image/base64_panel.h"
#include "pages/image/convert_image_panel.h"
#include "ui-kit/components/scroll_area.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/i18n.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include "widgets/drop_file_widget.h"
#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIODevice>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPushButton>
#include <QRectF>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSize>
#include <QStringList>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {

QString mimeTypeForExtension(const QString &ext) {
  if (ext == "svg") {
    return "image/svg+xml";
  }
  if (ext == "png") {
    return "image/png";
  }
  if (ext == "jpg" || ext == "jpeg") {
    return "image/jpeg";
  }
  if (ext == "webp") {
    return "image/webp";
  }
  if (ext == "gif") {
    return "image/gif";
  }
  if (ext == "bmp") {
    return "image/bmp";
  }
  if (ext == "ico") {
    return "image/x-icon";
  }
  return "application/octet-stream";
}

QSize probeImageDimensions(const QFileInfo &fi, const QByteArray &rawBytes) {
  if (fi.suffix().toLower() == "svg") {
    QSvgRenderer svg(rawBytes);
    if (!svg.isValid()) {
      return {};
    }
    QSize dim = svg.defaultSize();
    if (!dim.isValid() || dim.width() <= 0 || dim.height() <= 0) {
      dim = svg.viewBoxF().size().toSize();
    }
    return dim;
  }

  QImageReader reader(fi.absoluteFilePath());
  reader.setAutoTransform(true);
  QSize dim = reader.size();
  if (!dim.isValid()) {
    const QImage loaded = reader.read();
    dim = loaded.size();
  }
  return dim;
}

QString formatSnippetOutput(int snippetIdx, const QFileInfo &fi,
                            const QString &mimeType, const QSize &imgDim,
                            qint64 byteSize, const QString &b64,
                            const QString &dataUri) {
  if (snippetIdx == 1) {
    return b64;
  }
  if (snippetIdx == 2) {
    const int w = imgDim.width() > 0 ? imgDim.width() : 256;
    const int h = imgDim.height() > 0 ? imgDim.height() : 256;
    return QString("<img src=\"%1\" alt=\"%2\" width=\"%3\" height=\"%4\" />")
        .arg(dataUri, fi.completeBaseName().toHtmlEscaped())
        .arg(w)
        .arg(h);
  }
  if (snippetIdx == 3) {
    return QString("background-image: url(\"%1\");\n"
                   "background-repeat: no-repeat;\n"
                   "background-size: contain;")
        .arg(dataUri);
  }
  if (snippetIdx == 4) {
    QJsonObject obj;
    obj["fileName"] = fi.fileName();
    obj["mimeType"] = mimeType;
    if (imgDim.isValid() && imgDim.width() > 0 && imgDim.height() > 0) {
      obj["width"] = imgDim.width();
      obj["height"] = imgDim.height();
    }
    obj["byteSize"] = byteSize;
    obj["dataUri"] = dataUri;
    obj["base64"] = b64;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented))
        .trimmed();
  }
  return dataUri;
}

QString extractBase64Payload(const QString &rawInput, QString &outMimeHint) {
  QString text = rawInput;
  if (text.startsWith('{')) {
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(text.toUtf8());
    if (jsonDoc.isObject()) {
      const QJsonObject obj = jsonDoc.object();
      if (obj.contains("dataUri") && obj["dataUri"].isString()) {
        text = obj["dataUri"].toString().trimmed();
      } else if (obj.contains("base64") && obj["base64"].isString()) {
        text = obj["base64"].toString().trimmed();
      }
    }
  }

  outMimeHint.clear();
  const int dataIdx = text.indexOf("data:", 0, Qt::CaseInsensitive);
  const int b64MarkerIdx = text.indexOf(";base64,", 0, Qt::CaseInsensitive);
  if (dataIdx >= 0 && b64MarkerIdx > dataIdx) {
    outMimeHint = text.mid(dataIdx + 5, b64MarkerIdx - (dataIdx + 5))
                      .trimmed()
                      .toLower();
    const int startPos = b64MarkerIdx + 8;
    int endPos = text.size();
    for (int i = startPos; i < text.size(); ++i) {
      const QChar ch = text.at(i);
      if (ch == '"' || ch == '\'' || ch == ')' || ch == ';') {
        endPos = i;
        break;
      }
    }
    return text.mid(startPos, endPos - startPos).trimmed();
  }

  return text;
}

bool detectSvgOrRaster(const QByteArray &decodedBytes, const QString &mimeHint,
                       bool &outIsSvg, QImage &outImage) {
  outIsSvg = (mimeHint == "image/svg+xml");
  if (!outIsSvg) {
    const QByteArray trimmedSample = decodedBytes.left(512).trimmed();
    if (trimmedSample.startsWith("<svg") || trimmedSample.startsWith("<?xml")) {
      QSvgRenderer testSvg(decodedBytes);
      outIsSvg = testSvg.isValid();
    }
  }

  if (outIsSvg) {
    return true;
  }

  outImage = QImage::fromData(decodedBytes);
  if (!outImage.isNull()) {
    return true;
  }

  QSvgRenderer fallbackSvg(decodedBytes);
  if (fallbackSvg.isValid()) {
    outIsSvg = true;
    return true;
  }

  return false;
}

QString defaultExtensionFor(bool isSvgPayload, const QString &mimeHint) {
  if (isSvgPayload) {
    return "svg";
  }
  if (mimeHint == "image/jpeg" || mimeHint == "image/jpg") {
    return "jpg";
  }
  if (mimeHint == "image/webp") {
    return "webp";
  }
  if (mimeHint == "image/gif") {
    return "gif";
  }
  if (mimeHint == "image/bmp") {
    return "bmp";
  }
  if (mimeHint == "image/x-icon" ||
      mimeHint == "image/vnd.microsoft.icon") {
    return "ico";
  }
  return "png";
}

QString saveFilterFor(bool isSvgPayload, const QString &defaultExt) {
  if (isSvgPayload) {
    return Base64WorkspaceCard::tr(
        "SVG Vector (*.svg);;PNG Image (*.png);;WEBP Image "
        "(*.webp);;JPG Image (*.jpg)");
  }
  if (defaultExt == "ico") {
    return Base64WorkspaceCard::tr(
        "ICO Icon (*.ico);;PNG Image (*.png);;WEBP Image "
        "(*.webp);;JPG Image (*.jpg);;BMP Image (*.bmp)");
  }
  return Base64WorkspaceCard::tr(
      "PNG Image (*.png);;JPG Image (*.jpg);;WEBP Image "
      "(*.webp);;BMP Image (*.bmp);;TIFF Image (*.tiff)");
}

bool writeDecodedImageFile(const QString &outPath, const QString &outExt,
                           const QString &defaultExt, bool isSvgPayload,
                           const QByteArray &decodedBytes,
                           const QImage &decodedImg) {
  QSaveFile saveFile(outPath);
  saveFile.setDirectWriteFallback(false);
  if (!saveFile.open(QIODevice::WriteOnly)) {
    return false;
  }

  bool savedOk = false;
  if (isSvgPayload && outExt == "svg") {
    savedOk = (saveFile.write(decodedBytes) == decodedBytes.size());
  } else if (isSvgPayload) {
    QSvgRenderer renderer(decodedBytes);
    QSize targetSz = renderer.defaultSize();
    if (!targetSz.isValid() || targetSz.width() <= 0 ||
        targetSz.height() <= 0) {
      targetSz = QSize(512, 512);
    }
    QImage rendered(targetSz, outExt == "jpg"
                                  ? QImage::Format_RGB32
                                  : QImage::Format_ARGB32_Premultiplied);
    rendered.fill(outExt == "jpg" ? Qt::white : Qt::transparent);
    QPainter painter(&rendered);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter, QRectF(0, 0, targetSz.width(), targetSz.height()));
    painter.end();
    savedOk = rendered.save(&saveFile, outExt.toUpper().toUtf8().constData(),
                            outExt == "png" ? -1 : 90);
  } else if (outExt == defaultExt) {
    savedOk = (saveFile.write(decodedBytes) == decodedBytes.size());
  } else {
    savedOk =
        decodedImg.save(&saveFile, outExt.toUpper().toUtf8().constData(),
                        outExt == "png" ? -1 : 90);
  }

  return savedOk && saveFile.commit();
}

} // namespace

Base64OptionsPanel::Base64OptionsPanel(QWidget *parent) : QWidget(parent) {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  const QStringList snippetFormats = {
      tr("Raw Data URI (data:image/...;base64,...)"),
      tr("Raw Base64 Payload (no prefix)"),
      tr("HTML <img> Tag"),
      tr("CSS background-image"),
      tr("JSON Object Payload")};
  m_snippetSelect = new ui::Select(snippetFormats, this);
  m_snippetField =
      new ui::FormField(tr("Output Snippet Format"), m_snippetSelect, this);

  const QStringList encodeFormats = {
      tr("Keep Original File Bytes"), tr("Re-encode as PNG"),
      tr("Re-encode as WEBP"), tr("Re-encode as JPG")};
  m_encodeSelect = new ui::Select(encodeFormats, this);
  m_encodeField = new ui::FormField(tr("Encode Format"), m_encodeSelect, this);

  connect(m_snippetSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &Base64OptionsPanel::optionChanged);
  connect(m_encodeSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &Base64OptionsPanel::optionChanged);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &Base64OptionsPanel::retranslateUi);

  layout->addWidget(m_snippetField, 1, Qt::AlignTop);
  layout->addWidget(m_encodeField, 1, Qt::AlignTop);

  retranslateUi();
}

void Base64OptionsPanel::retranslateUi() {
  if (m_snippetField) {
    m_snippetField->setLabel(tr("Output Snippet Format"));
  }
  if (m_snippetSelect) {
    const QSignalBlocker blocker(m_snippetSelect);
    const QStringList snippetFormats = {
        tr("Raw Data URI (data:image/...;base64,...)"),
        tr("Raw Base64 Payload (no prefix)"),
        tr("HTML <img> Tag"),
        tr("CSS background-image"),
        tr("JSON Object Payload")};
    for (int i = 0; i < snippetFormats.size() && i < m_snippetSelect->count();
         ++i) {
      m_snippetSelect->setItemText(i, snippetFormats.at(i));
    }
  }
  if (m_encodeField) {
    m_encodeField->setLabel(tr("Encode Format"));
  }
  if (m_encodeSelect) {
    const QSignalBlocker blocker(m_encodeSelect);
    const QStringList encodeFormats = {
        tr("Keep Original File Bytes"), tr("Re-encode as PNG"),
        tr("Re-encode as WEBP"), tr("Re-encode as JPG")};
    for (int i = 0; i < encodeFormats.size() && i < m_encodeSelect->count();
         ++i) {
      m_encodeSelect->setItemText(i, encodeFormats.at(i));
    }
  }
}

int Base64OptionsPanel::snippetIndex() const {
  return m_snippetSelect ? m_snippetSelect->currentIndex() : 0;
}

int Base64OptionsPanel::encodeIndex() const {
  return m_encodeSelect ? m_encodeSelect->currentIndex() : 0;
}

Base64WorkspaceCard::Base64WorkspaceCard(QWidget *parent)
    : ui::Card(QString(), QString(), parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *content = new QWidget(this);
  content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  auto *layout = new QVBoxLayout(content);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  auto *toolbar = new QHBoxLayout();
  toolbar->setContentsMargins(0, 0, 0, 0);
  toolbar->setSpacing(8);

  m_loadBtn = new ui::Button(QString(), ui::ButtonVariant::Secondary,
                             ui::ButtonSize::Small, content);
  m_copyBtn = new ui::Button(QString(), ui::ButtonVariant::Outline,
                             ui::ButtonSize::Small, content);
  m_decodeSaveBtn = new ui::Button(QString(), ui::ButtonVariant::Outline,
                                   ui::ButtonSize::Small, content);
  m_statsLabel = new QLabel(content);

  connect(m_loadBtn, &QPushButton::clicked, this,
          &Base64WorkspaceCard::onLoadFileClicked);
  connect(m_copyBtn, &QPushButton::clicked, this,
          &Base64WorkspaceCard::copyToClipboard);
  connect(m_decodeSaveBtn, &QPushButton::clicked, this,
          &Base64WorkspaceCard::decodeAndSave);

  toolbar->addWidget(m_loadBtn);
  toolbar->addWidget(m_copyBtn);
  toolbar->addWidget(m_decodeSaveBtn);
  toolbar->addWidget(m_statsLabel, 1, Qt::AlignRight | Qt::AlignVCenter);

  m_textEdit = new QPlainTextEdit(content);
  m_textEdit->setMinimumHeight(130);
  m_textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  layout->addLayout(toolbar);
  layout->addWidget(m_textEdit, 1);
  setContentWidget(content);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &Base64WorkspaceCard::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &Base64WorkspaceCard::retranslateUi);
  applyThemeStyles();
  retranslateUi();
}

void Base64WorkspaceCard::retranslateUi() {
  setTitle(tr("Image ↔ Base64 Data URI Workspace"));
  setDescription(
      tr("Load an image or SVG to generate inline Data URIs, HTML, CSS, "
         "or JSON snippets — or paste Base64 below to decode back to an "
         "image file."));
  if (m_loadBtn) {
    m_loadBtn->setText(tr("Load Image / SVG..."));
  }
  if (m_copyBtn) {
    m_copyBtn->setText(tr("Copy to Clipboard"));
  }
  if (m_decodeSaveBtn) {
    m_decodeSaveBtn->setText(tr("Decode Base64 & Save Image..."));
  }
  if (m_textEdit) {
    m_textEdit->setPlaceholderText(
        tr("Click 'Load Image / SVG...' to encode a file into Base64 / Data "
           "URI, or paste a data:image/...;base64,... URI or raw Base64 "
           "payload here and click 'Decode Base64 & Save Image...'."));
  }
  updateStatsLabel();
}

void Base64WorkspaceCard::updateStatsLabel() {
  if (!m_statsLabel) {
    return;
  }
  if (m_loadedFilePath.isEmpty()) {
    m_statsLabel->setText(
        tr("No image loaded — select a file or paste Base64 below"));
    return;
  }
  const QString dimStr =
      (m_lastStatsDim.isValid() && m_lastStatsDim.width() > 0 &&
       m_lastStatsDim.height() > 0)
          ? tr("%1×%2 px • ")
                .arg(m_lastStatsDim.width())
                .arg(m_lastStatsDim.height())
          : QString();
  m_statsLabel->setText(
      tr("%1 (%2%3 → %4 chars)")
          .arg(m_lastStatsFileName, dimStr,
               DropFileWidget::formatFileSize(m_lastStatsByteSize),
               QString::number(m_lastStatsCharCount)));
}

void Base64WorkspaceCard::setOptionsPanel(Base64OptionsPanel *optionsPanel) {
  if (m_optionsPanel == optionsPanel) {
    return;
  }
  if (m_optionsPanel) {
    disconnect(m_optionsPanel, &Base64OptionsPanel::optionChanged, this,
               &Base64WorkspaceCard::onOptionChanged);
  }
  m_optionsPanel = optionsPanel;
  if (m_optionsPanel) {
    connect(m_optionsPanel, &Base64OptionsPanel::optionChanged, this,
            &Base64WorkspaceCard::onOptionChanged, Qt::UniqueConnection);
  }
}

void Base64WorkspaceCard::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  if (m_statsLabel) {
    m_statsLabel->setFont(t.font(t.sizeXs, QFont::Medium));
    m_statsLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_textEdit) {
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPixelSize(t.sizeXs);
    m_textEdit->setFont(monoFont);
    m_textEdit->setStyleSheet(
        QString("QPlainTextEdit {"
                "  background-color: %1;"
                "  color: %2;"
                "  border: 1px solid %3;"
                "  border-radius: %4px;"
                "  padding: 6px 8px;"
                "}")
            .arg(ui::StyleHelper::toHexString(c.background))
            .arg(ui::StyleHelper::toHexString(c.foreground))
            .arg(ui::StyleHelper::toHexString(c.border))
            .arg(r.md));
    ui::ScrollArea::applyScrollBarStyles(m_textEdit);
  }
}

QWidget *Base64WorkspaceCard::toastHost() {
  return window() ? window() : this;
}

void Base64WorkspaceCard::onLoadFileClicked() {
  const QString filePath = QFileDialog::getOpenFileName(
      this, tr("Select Image or SVG to Encode"), QDir::homePath(),
      tr("Images & Vector Files (*.png *.jpg *.jpeg *.webp *.svg *.gif *.bmp "
         "*.ico);;All Files (*)"));
  if (filePath.isEmpty()) {
    return;
  }
  loadFileIntoBase64(filePath);
}

void Base64WorkspaceCard::onOptionChanged() {
  if (!m_loadedFilePath.isEmpty() && QFile::exists(m_loadedFilePath)) {
    loadFileIntoBase64(m_loadedFilePath);
  }
}

void Base64WorkspaceCard::loadFileIntoBase64(const QString &filePath) {
  const QFileInfo fi(filePath);
  QFile rawFile(fi.absoluteFilePath());
  if (!rawFile.open(QIODevice::ReadOnly)) {
    ui::Toast::show(toastHost(), tr("Read Error"),
                    tr("Cannot open '%1'.").arg(fi.fileName()),
                    ui::ToastVariant::Destructive);
    return;
  }
  const QByteArray rawBytes = rawFile.readAll();
  rawFile.close();

  const int encodeIdx = m_optionsPanel ? m_optionsPanel->encodeIndex() : 0;
  const QString ext = fi.suffix().toLower();
  QSize imgDim;

  QByteArray payloadBytes;
  QString mimeType = "image/png";

  if (encodeIdx == 0) {
    imgDim = probeImageDimensions(fi, rawBytes);
    payloadBytes = rawBytes;
    mimeType = mimeTypeForExtension(ext);
  } else {
    const bool forceWhite = (encodeIdx == 3); // JPG
    const QImage img = ConvertImagePanel::loadRasterOrSvgImage(
        fi.absoluteFilePath(), forceWhite);
    if (img.isNull()) {
      ui::Toast::show(toastHost(), tr("Encode Error"),
                      tr("Could not decode image for re-encoding."),
                      ui::ToastVariant::Destructive);
      return;
    }
    imgDim = img.size();
    QBuffer buf(&payloadBytes);
    bool encodedOk = buf.open(QIODevice::WriteOnly);
    if (encodedOk) {
      if (encodeIdx == 1) {
        mimeType = "image/png";
        encodedOk = img.save(&buf, "PNG", -1);
      } else if (encodeIdx == 2) {
        mimeType = "image/webp";
        encodedOk = img.save(&buf, "WEBP", 90);
      } else {
        mimeType = "image/jpeg";
        encodedOk = img.save(&buf, "JPG", 90);
      }
      buf.close();
    }
    if (!encodedOk || payloadBytes.isEmpty()) {
      ui::Toast::show(toastHost(), tr("Encode Error"),
                      tr("Failed to re-encode image into target format."),
                      ui::ToastVariant::Destructive);
      return;
    }
  }

  m_loadedFilePath = fi.absoluteFilePath();

  const QString b64 = QString::fromLatin1(payloadBytes.toBase64());
  const QString dataUri = QString("data:%1;base64,%2").arg(mimeType, b64);
  const int snippetIdx = m_optionsPanel ? m_optionsPanel->snippetIndex() : 0;
  const QString formattedOutput =
      formatSnippetOutput(snippetIdx, fi, mimeType, imgDim,
                          static_cast<qint64>(payloadBytes.size()), b64,
                          dataUri);

  if (m_textEdit) {
    m_textEdit->setPlainText(formattedOutput);
  }
  m_lastStatsFileName = fi.fileName();
  m_lastStatsDim = imgDim;
  m_lastStatsByteSize = static_cast<qint64>(payloadBytes.size());
  m_lastStatsCharCount = formattedOutput.size();
  updateStatsLabel();
}

void Base64WorkspaceCard::copyToClipboard() {
  const QString text =
      m_textEdit ? m_textEdit->toPlainText().trimmed() : QString();
  if (text.isEmpty()) {
    ui::Toast::show(toastHost(), tr("Nothing to Copy"),
                    tr("Load an image or enter a Base64 snippet first."),
                    ui::ToastVariant::Warning);
    return;
  }
  if (QClipboard *cb = QGuiApplication::clipboard()) {
    cb->setText(text);
    ui::Toast::show(
        toastHost(), tr("Copied to Clipboard"),
        tr("Copied %1 characters to clipboard.").arg(text.size()),
        ui::ToastVariant::Success);
  }
}

void Base64WorkspaceCard::decodeAndSave() {
  const QString rawText =
      m_textEdit ? m_textEdit->toPlainText().trimmed() : QString();
  if (rawText.isEmpty()) {
    ui::Toast::show(toastHost(), tr("Empty Base64 Input"),
                    tr("Paste a Data URI or Base64 string into the editor first."),
                    ui::ToastVariant::Warning);
    return;
  }

  QString mimeHint;
  const QString base64Payload = extractBase64Payload(rawText, mimeHint);
  const QByteArray decodedBytes =
      QByteArray::fromBase64(base64Payload.toLatin1());
  if (decodedBytes.isEmpty()) {
    ui::Toast::show(toastHost(), tr("Invalid Base64"),
                    tr("The input could not be decoded from Base64."),
                    ui::ToastVariant::Destructive);
    return;
  }

  bool isSvgPayload = false;
  QImage decodedImg;
  if (!detectSvgOrRaster(decodedBytes, mimeHint, isSvgPayload, decodedImg)) {
    ui::Toast::show(
        toastHost(), tr("Unsupported Image Data"),
        tr("Decoded bytes do not form a valid raster image or SVG document."),
        ui::ToastVariant::Destructive);
    return;
  }

  const QString defaultExt = defaultExtensionFor(isSvgPayload, mimeHint);
  const QString defaultPath =
      QDir(QDir::homePath()).filePath("decoded_image." + defaultExt);
  const QString saveFilter = saveFilterFor(isSvgPayload, defaultExt);

  QString outPath = QFileDialog::getSaveFileName(this, tr("Save Decoded Image"),
                                                 defaultPath, saveFilter);
  if (outPath.isEmpty()) {
    return;
  }
  if (QFileInfo(outPath).suffix().isEmpty()) {
    outPath += "." + defaultExt;
  }

  const QString outExt = QFileInfo(outPath).suffix().toLower();
  if (writeDecodedImageFile(outPath, outExt, defaultExt, isSvgPayload,
                            decodedBytes, decodedImg)) {
    ui::Toast::show(toastHost(), tr("Saved Decoded Image"),
                    tr("Saved to %1").arg(QFileInfo(outPath).fileName()),
                    ui::ToastVariant::Success);
  } else {
    ui::Toast::show(toastHost(), tr("Save Failed"),
                    tr("Could not save '%1'.").arg(outPath),
                    ui::ToastVariant::Destructive);
  }
}
