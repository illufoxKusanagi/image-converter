#include "input.h"
#include "../theme/style_helper.h"

namespace ui {

Input::Input(QWidget *parent)
    : Input("", parent) {}

Input::Input(const QString &placeholder, QWidget *parent)
    : QWidget(parent) {
  setFixedHeight(40);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  m_layout = new QHBoxLayout(this);
  m_layout->setContentsMargins(12, 0, 8, 0);
  m_layout->setSpacing(6);

  m_prefixLabel = new QLabel(this);
  m_prefixLabel->setVisible(false);

  m_lineEdit = new QLineEdit(this);
  m_lineEdit->setFrame(false);
  m_lineEdit->setPlaceholderText(placeholder);
  m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  m_suffixLabel = new QLabel(this);
  m_suffixLabel->setVisible(false);

  m_clearButton = new QPushButton(QString::fromUtf8("\u2715"), this);
  m_clearButton->setFixedSize(18, 18);
  m_clearButton->setCursor(Qt::PointingHandCursor);
  m_clearButton->setToolTip("Clear");
  m_clearButton->setVisible(false);
  connect(m_clearButton, &QPushButton::clicked, m_lineEdit, &QLineEdit::clear);

  m_layout->addWidget(m_prefixLabel);
  m_layout->addWidget(m_lineEdit, 1);
  m_layout->addWidget(m_suffixLabel);
  m_layout->addWidget(m_clearButton);

  connect(m_lineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
    if (m_clearButtonEnabled) {
      m_clearButton->setVisible(!text.isEmpty());
    }
    emit textChanged(text);
  });
  connect(m_lineEdit, &QLineEdit::textEdited, this, &Input::textEdited);
  connect(m_lineEdit, &QLineEdit::returnPressed, this, &Input::returnPressed);

  connect(&Theme::instance(), &Theme::themeChanged, this, &Input::applyThemeStyles);
  applyThemeStyles();
}

QString Input::text() const {
  return m_lineEdit->text();
}

void Input::setText(const QString &text) {
  m_lineEdit->setText(text);
}

QString Input::placeholderText() const {
  return m_lineEdit->placeholderText();
}

void Input::setPlaceholderText(const QString &placeholder) {
  m_lineEdit->setPlaceholderText(placeholder);
}

void Input::setPrefixText(const QString &prefix) {
  m_prefixLabel->setText(prefix);
  m_prefixLabel->setVisible(!prefix.isEmpty());
}

void Input::setSuffixText(const QString &suffix) {
  m_suffixLabel->setText(suffix);
  m_suffixLabel->setVisible(!suffix.isEmpty());
}

bool Input::isClearButtonEnabled() const {
  return m_clearButtonEnabled;
}

void Input::setClearButtonEnabled(bool enable) {
  m_clearButtonEnabled = enable;
  m_clearButton->setVisible(enable && !m_lineEdit->text().isEmpty());
}

bool Input::isReadOnly() const {
  return m_lineEdit->isReadOnly();
}

void Input::setReadOnly(bool readOnly) {
  m_lineEdit->setReadOnly(readOnly);
  applyThemeStyles();
}

bool Input::hasError() const {
  return m_hasError;
}

void Input::setError(bool hasError) {
  if (m_hasError != hasError) {
    m_hasError = hasError;
    applyThemeStyles();
  }
}

QLineEdit *Input::lineEdit() const {
  return m_lineEdit;
}

void Input::applyThemeStyles() {
  const auto &c = Theme::instance().colors();
  const auto &r = Theme::instance().radius();
  const auto &t = Theme::instance().typography();

  setFont(t.font(t.sizeBase, QFont::Normal));
  m_lineEdit->setFont(t.font(t.sizeBase, QFont::Normal));
  m_prefixLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
  m_suffixLabel->setFont(t.font(t.sizeSm, QFont::Normal));

  QColor borderColor = m_hasError ? c.destructive : c.border;
  QColor focusRingColor = m_hasError ? c.destructive : c.ring;

  setStyleSheet(QString(
      "ui--Input {"
      "  background-color: %1;"
      "  border: 1px solid %2;"
      "  border-radius: %3px;"
      "}"
      "ui--Input:hover {"
      "  border-color: %4;"
      "}"
      "ui--Input:focus-within {"
      "  border: 2px solid %5;"
      "}")
      .arg(StyleHelper::toHexString(c.background))
      .arg(StyleHelper::toHexString(borderColor))
      .arg(r.md)
      .arg(StyleHelper::toHexString(c.primary))
      .arg(StyleHelper::toHexString(focusRingColor)));

  m_lineEdit->setStyleSheet(QString(
      "QLineEdit {"
      "  background: transparent;"
      "  border: none;"
      "  color: %1;"
      "  selection-background-color: %2;"
      "  selection-color: %3;"
      "}")
      .arg(StyleHelper::toHexString(c.foreground))
      .arg(StyleHelper::toHexString(c.primary))
      .arg(StyleHelper::toHexString(c.primaryForeground)));

  m_prefixLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                   .arg(StyleHelper::toHexString(c.mutedForeground)));
  m_suffixLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                   .arg(StyleHelper::toHexString(c.mutedForeground)));

  m_clearButton->setStyleSheet(QString(
      "QPushButton {"
      "  background: transparent;"
      "  border: none;"
      "  color: %1;"
      "  font-size: 11px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  color: %2;"
      "}")
      .arg(StyleHelper::toHexString(c.mutedForeground))
      .arg(StyleHelper::toHexString(c.destructive)));
}

} // namespace ui
