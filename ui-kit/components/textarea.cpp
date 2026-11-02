#include "textarea.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QEvent>
#include <QPalette>

namespace ui {

Textarea::Textarea(QWidget *parent) : Textarea(QString(), parent) {}

Textarea::Textarea(const QString &placeholder, QWidget *parent)
    : QPlainTextEdit(parent) {
  setPlaceholderText(placeholder);
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Textarea::applyThemeStyles);
  applyThemeStyles();
}

bool Textarea::hasError() const { return m_hasError; }

void Textarea::setError(bool hasError) {
  if (m_hasError == hasError) {
    return;
  }
  m_hasError = hasError;
  applyThemeStyles();
}

bool Textarea::event(QEvent *event) {
  const bool handled = QPlainTextEdit::event(event);
  if (event->type() == QEvent::ReadOnlyChange) {
    applyThemeStyles();
  }
  return handled;
}

void Textarea::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &r = theme.radius();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();
  const QColor background = isReadOnly() ? c.muted : c.background;
  const QColor border = m_hasError ? c.destructive : c.input;
  const QColor hoverBorder =
      m_hasError ? c.destructive : (isReadOnly() ? c.input : c.primary);
  const QColor focusBorder = m_hasError ? c.destructive : c.ring;

  setFont(t.font(t.sizeBase));
  QPalette colors = palette();
  colors.setColor(QPalette::Base, background);
  colors.setColor(QPalette::Text, c.foreground);
  colors.setColor(QPalette::Highlight, c.primary);
  colors.setColor(QPalette::HighlightedText, c.primaryForeground);
  colors.setColor(QPalette::Disabled, QPalette::Base, c.muted);
  colors.setColor(QPalette::Disabled, QPalette::Text, c.mutedForeground);
  colors.setColor(QPalette::Disabled, QPalette::Highlight, c.secondary);
  colors.setColor(QPalette::Disabled, QPalette::HighlightedText,
                  c.mutedForeground);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
  colors.setColor(QPalette::PlaceholderText, c.mutedForeground);
#endif
  setPalette(colors);

  setStyleSheet(QString("ui--Textarea {"
                        "  background-color: %1;"
                        "  color: %2;"
                        "  border: 1px solid %3;"
                        "  border-radius: %4px;"
                        "  padding: %5px %6px;"
                        "  selection-background-color: %7;"
                        "  selection-color: %8;"
                        "}"
                        "ui--Textarea:hover:enabled { border-color: %9; }"
                        "ui--Textarea:focus { border-color: %10; }"
                        "ui--Textarea:disabled {"
                        "  background-color: %11;"
                        "  color: %12;"
                        "  border-color: %13;"
                        "  selection-background-color: %14;"
                        "  selection-color: %12;"
                        "}")
                    .arg(StyleHelper::toHexString(background))
                    .arg(StyleHelper::toHexString(c.foreground))
                    .arg(StyleHelper::toHexString(border))
                    .arg(r.md)
                    .arg(s.space2)
                    .arg(s.space3)
                    .arg(StyleHelper::toHexString(c.primary))
                    .arg(StyleHelper::toHexString(c.primaryForeground))
                    .arg(StyleHelper::toHexString(hoverBorder))
                    .arg(StyleHelper::toHexString(focusBorder))
                    .arg(StyleHelper::toHexString(c.muted))
                    .arg(StyleHelper::toHexString(c.mutedForeground))
                    .arg(StyleHelper::toHexString(c.border))
                    .arg(StyleHelper::toHexString(c.secondary)));
}

} // namespace ui
