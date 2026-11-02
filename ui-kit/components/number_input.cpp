#include "number_input.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QEvent>
#include <QLineEdit>
#include <QPalette>

namespace ui {
namespace {

void applySpinBoxTheme(QAbstractSpinBox *spinBox, QLineEdit *editor,
                       const QString &selector, bool hasError) {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &r = theme.radius();
  const auto &s = theme.spacing();
  const auto &t = theme.typography();
  const bool isDark = theme.isDark();
  const bool readOnly = editor->isReadOnly();
  const QColor background = readOnly ? c.muted : c.background;
  const QColor border =
      hasError ? c.destructive
               : (readOnly ? c.border
                           : StyleHelper::tint(c.border, c.foreground, 0.25));
  const QColor hoverBorder =
      hasError ? c.destructive : (readOnly ? c.border : c.primary);
  const QColor focusBorder = hasError ? c.destructive : c.ring;
  const QColor compartmentBg =
      readOnly ? c.muted
               : (isDark ? c.secondary
                         : StyleHelper::tint(c.background, c.primary, 0.08));
  const QColor compartmentHover =
      readOnly ? c.muted
               : (isDark ? StyleHelper::lighten(c.secondary, 18)
                         : StyleHelper::tint(c.background, c.primary, 0.18));
  const QColor compartmentPressed =
      readOnly ? c.muted
               : (isDark ? StyleHelper::lighten(c.secondary, 30)
                         : StyleHelper::tint(c.background, c.primary, 0.28));
  const QString upIcon =
      isDark ? QStringLiteral(":/icons/icons/arrow-up.svg")
             : QStringLiteral(":/icons/icons/arrow-up-black.svg");
  const QString downIcon =
      isDark ? QStringLiteral(":/icons/icons/arrow-down.svg")
             : QStringLiteral(":/icons/icons/arrow-down-black.svg");

  spinBox->setFont(t.font(t.sizeBase));
  spinBox->setMinimumHeight(s.space10);
  QPalette colors = spinBox->palette();
  colors.setColor(QPalette::Window, background);
  colors.setColor(QPalette::WindowText, c.foreground);
  colors.setColor(QPalette::Base, background);
  colors.setColor(QPalette::Text, c.foreground);
  colors.setColor(QPalette::Button, c.secondary);
  colors.setColor(QPalette::ButtonText, c.secondaryForeground);
  colors.setColor(QPalette::Light, c.background);
  colors.setColor(QPalette::Midlight, c.secondary);
  colors.setColor(QPalette::Mid, c.border);
  colors.setColor(QPalette::Dark, c.input);
  colors.setColor(QPalette::Shadow, c.border);
  colors.setColor(QPalette::Highlight, c.primary);
  colors.setColor(QPalette::HighlightedText, c.primaryForeground);
  colors.setColor(QPalette::Disabled, QPalette::Window, c.muted);
  colors.setColor(QPalette::Disabled, QPalette::WindowText, c.mutedForeground);
  colors.setColor(QPalette::Disabled, QPalette::Base, c.muted);
  colors.setColor(QPalette::Disabled, QPalette::Text, c.mutedForeground);
  colors.setColor(QPalette::Disabled, QPalette::Button, c.muted);
  colors.setColor(QPalette::Disabled, QPalette::ButtonText, c.mutedForeground);
  colors.setColor(QPalette::Disabled, QPalette::Highlight, c.secondary);
  colors.setColor(QPalette::Disabled, QPalette::HighlightedText,
                  c.mutedForeground);
  spinBox->setPalette(colors);
  editor->setFont(t.font(t.sizeBase));
  editor->setPalette(colors);

  spinBox->setStyleSheet(
      QString("%1 {"
              "  background-color: %2;"
              "  color: %3;"
              "  border: 1px solid %4;"
              "  border-radius: %5px;"
              "  padding: 0px 36px 0px %7px;"
              "  selection-background-color: %8;"
              "  selection-color: %9;"
              "}"
              "%1:hover:enabled { border-color: %10; }"
              "%1:focus { border-color: %11; }"
              "%1:disabled {"
              "  background-color: %12;"
              "  color: %13;"
              "  border-color: %14;"
              "  selection-background-color: %15;"
              "  selection-color: %13;"
              "}"
              "%1::up-button {"
              "  subcontrol-origin: border;"
              "  subcontrol-position: top right;"
              "  width: 30px;"
              "  border-left: 1px solid %4;"
              "  border-bottom: 1px solid %4;"
              "  border-top: 1px solid %4;"
              "  border-right: 1px solid %4;"
              "  border-top-right-radius: %6px;"
              "  background-color: %16;"
              "}"
              "%1::down-button {"
              "  subcontrol-origin: border;"
              "  subcontrol-position: bottom right;"
              "  width: 30px;"
              "  border-left: 1px solid %4;"
              "  border-bottom: 1px solid %4;"
              "  border-right: 1px solid %4;"
              "  border-top: none;"
              "  border-bottom-right-radius: %6px;"
              "  background-color: %16;"
              "}"
              "%1:hover:enabled::up-button, %1:hover:enabled::down-button {"
              "  border-color: %10;"
              "}"
              "%1:focus::up-button, %1:focus::down-button {"
              "  border-color: %11;"
              "}"
              "%1::up-button:hover:enabled, %1::down-button:hover:enabled {"
              "  background-color: %17;"
              "}"
              "%1::up-button:pressed:enabled, %1::down-button:pressed:enabled {"
              "  background-color: %18;"
              "}"
              "%1::up-button:disabled, %1::down-button:disabled {"
              "  background-color: %12;"
              "  border-color: %14;"
              "}"
              "%1::up-arrow {"
              "  image: url(%19);"
              "  width: 10px;"
              "  height: 10px;"
              "}"
              "%1::down-arrow {"
              "  image: url(%20);"
              "  width: 10px;"
              "  height: 10px;"
              "}")
          .arg(selector)
          .arg(StyleHelper::toHexString(background))
          .arg(StyleHelper::toHexString(c.foreground))
          .arg(StyleHelper::toHexString(border))
          .arg(r.md)
          .arg(r.md - 1)
          .arg(s.space3)
          .arg(StyleHelper::toHexString(c.primary))
          .arg(StyleHelper::toHexString(c.primaryForeground))
          .arg(StyleHelper::toHexString(hoverBorder))
          .arg(StyleHelper::toHexString(focusBorder))
          .arg(StyleHelper::toHexString(c.muted))
          .arg(StyleHelper::toHexString(c.mutedForeground))
          .arg(StyleHelper::toHexString(c.border))
          .arg(StyleHelper::toHexString(c.secondary))
          .arg(StyleHelper::toHexString(compartmentBg))
          .arg(StyleHelper::toHexString(compartmentHover))
          .arg(StyleHelper::toHexString(compartmentPressed))
          .arg(upIcon)
          .arg(downIcon));
}

} // namespace

NumberInput::NumberInput(QWidget *parent) : QSpinBox(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  lineEdit()->installEventFilter(this);
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &NumberInput::applyThemeStyles);
  applyThemeStyles();
}

bool NumberInput::hasError() const { return m_hasError; }

void NumberInput::setError(bool hasError) {
  if (m_hasError == hasError) {
    return;
  }
  m_hasError = hasError;
  applyThemeStyles();
}

bool NumberInput::eventFilter(QObject *watched, QEvent *event) {
  const bool handled = QSpinBox::eventFilter(watched, event);
  if (watched == lineEdit() && event->type() == QEvent::ReadOnlyChange) {
    applyThemeStyles();
  }
  return handled;
}

void NumberInput::applyThemeStyles() {
  applySpinBoxTheme(this, lineEdit(), QStringLiteral("ui--NumberInput"),
                    m_hasError);
}

DecimalInput::DecimalInput(QWidget *parent) : QDoubleSpinBox(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  lineEdit()->installEventFilter(this);
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &DecimalInput::applyThemeStyles);
  applyThemeStyles();
}

bool DecimalInput::hasError() const { return m_hasError; }

void DecimalInput::setError(bool hasError) {
  if (m_hasError == hasError) {
    return;
  }
  m_hasError = hasError;
  applyThemeStyles();
}

bool DecimalInput::eventFilter(QObject *watched, QEvent *event) {
  const bool handled = QDoubleSpinBox::eventFilter(watched, event);
  if (watched == lineEdit() && event->type() == QEvent::ReadOnlyChange) {
    applyThemeStyles();
  }
  return handled;
}

void DecimalInput::applyThemeStyles() {
  applySpinBoxTheme(this, lineEdit(), QStringLiteral("ui--DecimalInput"),
                    m_hasError);
}

} // namespace ui
