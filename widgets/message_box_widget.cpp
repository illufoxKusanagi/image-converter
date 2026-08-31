#include "message_box_widget.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"

MessageBoxWidget::MessageBoxWidget(const QString &windowTitle,
                                   const QString &text, IconType iconType,
                                   QWidget *parent)
    : QMessageBox(parent) {
  setWindowTitle(windowTitle);
  setText(text);
  setIcon(switchIcon(iconType));
  setStandardButtons(QMessageBox::Ok);

  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();

  setFont(t.font(t.sizeSm, QFont::Normal));
  setStyleSheet(QString(
      "QMessageBox {"
      "  background-color: %1;"
      "}"
      "QMessageBox QLabel {"
      "  color: %2;"
      "}"
      "QPushButton {"
      "  background-color: %3;"
      "  color: %4;"
      "  border: none;"
      "  padding: 6px 16px;"
      "  border-radius: %5px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  background-color: %6;"
      "}")
      .arg(ui::StyleHelper::toHexString(c.card))
      .arg(ui::StyleHelper::toHexString(c.foreground))
      .arg(ui::StyleHelper::toHexString(c.primary))
      .arg(ui::StyleHelper::toHexString(c.primaryForeground))
      .arg(r.sm)
      .arg(ui::StyleHelper::toHexString(ui::StyleHelper::hoverColor(c.primary, ui::Theme::instance().isDark()))));
}

constexpr QMessageBox::Icon MessageBoxWidget::switchIcon(IconType iconType) {
  switch (iconType) {
  case NoIcon:
    return QMessageBox::NoIcon;
  case Question:
    return QMessageBox::Question;
  case Information:
    return QMessageBox::Information;
  case Warning:
    return QMessageBox::Warning;
  case Critical:
    return QMessageBox::Critical;
  default:
    return QMessageBox::NoIcon;
  }
}