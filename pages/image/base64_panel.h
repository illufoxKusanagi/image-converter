#ifndef BASE64_PANEL_H
#define BASE64_PANEL_H

#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/select.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QString>
#include <QWidget>

/**
 * @brief Options panel for configuring Base64 / Data URI encoding output.
 *
 * Provides selectors for the formatted output snippet style (Data URI, raw
 * Base64, HTML <img> tag, CSS background-image, or JSON payload) and for
 * preserving original bytes or re-encoding to PNG, WEBP, or JPG.
 */
class Base64OptionsPanel : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Base64 options panel.
   * @param parent Optional Qt parent widget.
   */
  explicit Base64OptionsPanel(QWidget *parent = nullptr);
  ~Base64OptionsPanel() override = default;

  /**
   * @brief Returns the active output snippet format selector index.
   * @return 0 for Data URI, 1 for Raw Base64, 2 for HTML <img>, 3 for CSS,
   *         4 for JSON payload.
   */
  int snippetIndex() const;

  /**
   * @brief Returns the active encode format selector index.
   * @return 0 for Keep Original File Bytes, 1 for PNG, 2 for WEBP, 3 for JPG.
   */
  int encodeIndex() const;

signals:
  /**
   * @brief Emitted when either the snippet format or encode format changes.
   */
  void optionChanged();

private:
  ui::Select *m_snippetSelect{nullptr};
  ui::FormField *m_snippetField{nullptr};
  ui::Select *m_encodeSelect{nullptr};
  ui::FormField *m_encodeField{nullptr};
};

/**
 * @brief Interactive workspace card for encoding images/SVGs to Base64 and
 *        decoding Base64/Data URI snippets back into image files.
 */
class Base64WorkspaceCard : public ui::Card {
  Q_OBJECT

public:
  /**
   * @brief Constructs the Base64 workspace card and wires toolbar actions.
   * @param parent Optional Qt parent widget.
   */
  explicit Base64WorkspaceCard(QWidget *parent = nullptr);
  ~Base64WorkspaceCard() override = default;

  /**
   * @brief Associates an options panel whose changes trigger live re-encoding.
   * @param optionsPanel Options panel providing snippet and encode selections.
   */
  void setOptionsPanel(Base64OptionsPanel *optionsPanel);

public slots:
  /**
   * @brief Applies current theme typography and colors to child controls.
   */
  void applyThemeStyles();

  /**
   * @brief Copies the current trimmed workspace text to the system clipboard.
   */
  void copyToClipboard();

  /**
   * @brief Decodes the Base64, Data URI, HTML/CSS snippet, or JSON payload in
   *        the editor and prompts to save the decoded raster image or SVG.
   */
  void decodeAndSave();

private slots:
  void onLoadFileClicked();
  void onOptionChanged();

private:
  void loadFileIntoBase64(const QString &filePath);
  QWidget *toastHost();

  QPointer<Base64OptionsPanel> m_optionsPanel;
  ui::Button *m_loadBtn{nullptr};
  ui::Button *m_copyBtn{nullptr};
  ui::Button *m_decodeSaveBtn{nullptr};
  QLabel *m_statsLabel{nullptr};
  QPlainTextEdit *m_textEdit{nullptr};
  QString m_loadedFilePath;
};

#endif // BASE64_PANEL_H
