#include "input_dropdown.h"
#include "ui-kit/theme/style_helper.h"
#include <QListView>

InputDropdown::InputDropdown(QWidget *parent, const QStringList options)
    : QWidget(parent), m_dropdown(new QComboBox(this)), m_options(options) {
  connect(m_dropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int) { emit valueChanged(); });
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  m_dropdown->setView(new QListView(this));
  m_dropdown->addItems(options);
  m_dropdown->setFixedHeight(38);
  m_dropdown->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  layout->addWidget(m_dropdown);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this, &InputDropdown::applyThemeStyles);
  applyThemeStyles();
}

void InputDropdown::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &r = ui::Theme::instance().radius();
  const auto &t = ui::Theme::instance().typography();
  bool isDark = ui::Theme::instance().isDark();

  m_dropdown->setFont(t.font(t.sizeBase, QFont::Normal));

  QString bgHex = ui::StyleHelper::toHexString(c.background);
  QString fgHex = ui::StyleHelper::toHexString(c.foreground);
  QString borderHex = ui::StyleHelper::toHexString(c.primary);
  QString hoverBorderHex = ui::StyleHelper::toHexString(ui::StyleHelper::hoverColor(c.primary, isDark));
  QString popoverHex = ui::StyleHelper::toHexString(c.popover);
  QString popoverFgHex = ui::StyleHelper::toHexString(c.popoverForeground);
  QString accentHex = ui::StyleHelper::toHexString(c.accent);
  QString accentFgHex = ui::StyleHelper::toHexString(c.accentForeground);
  QString mutedHex = ui::StyleHelper::toHexString(c.muted);
  QString mutedFgHex = ui::StyleHelper::toHexString(c.mutedForeground);
  QString compartmentBgHex = isDark ? "#27272a" : "#ede9fe";
  QString compartmentHoverHex = isDark ? "#3f3f46" : "#ddd6fe";
  QString compartmentPressedHex = isDark ? "#52525b" : "#c4b5fd";
  QString downIcon = isDark ? ":/icons/icons/arrow-down.svg" : ":/icons/icons/arrow-down-black.svg";

  m_dropdown->setStyleSheet(
      "QComboBox {"
      "  background-color: " + bgHex + ";"
      "  color: " + fgHex + ";"
      "  border: 1px solid " + borderHex + ";"
      "  border-radius: " + QString::number(r.md) + "px;"
      "  padding: 0px 36px 0px 12px;"
      "}"
      "QComboBox:hover, QComboBox:focus {"
      "  border-color: " + hoverBorderHex + ";"
      "}"
      "QComboBox::drop-down {"
      "  subcontrol-origin: border;"
      "  subcontrol-position: top right;"
      "  width: 32px;"
      "  border: 1px solid " + borderHex + ";"
      "  border-top-right-radius: " + QString::number(r.md - 1) + "px;"
      "  border-bottom-right-radius: " + QString::number(r.md - 1) + "px;"
      "  background-color: " + compartmentBgHex + ";"
      "}"
      "QComboBox:hover::drop-down, QComboBox:focus::drop-down {"
      "  border-color: " + hoverBorderHex + ";"
      "}"
      "QComboBox::drop-down:hover {"
      "  background-color: " + compartmentHoverHex + ";"
      "}"
      "QComboBox::drop-down:pressed {"
      "  background-color: " + compartmentPressedHex + ";"
      "}"
      "QComboBox::down-arrow {"
      "  image: url(" + downIcon + ");"
      "  width: 10px;"
      "  height: 10px;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background-color: " + popoverHex + ";"
      "  color: " + popoverFgHex + ";"
      "  border: 1px solid " + borderHex + ";"
      "  border-radius: 8px;"
      "  padding: 4px;"
      "  outline: none;"
      "}"
      "QComboBox QAbstractItemView::item {"
      "  min-height: 30px;"
      "  padding: 6px 12px;"
      "  margin: 2px 4px;"
      "  border-radius: 6px;"
      "  color: " + popoverFgHex + ";"
      "}"
      "QComboBox QAbstractItemView::item:hover, QComboBox QAbstractItemView::item:selected {"
      "  background-color: " + accentHex + ";"
      "  color: " + accentFgHex + ";"
      "  border-radius: 6px;"
      "}"
      "QComboBox:disabled {"
      "  background-color: " + mutedHex + ";"
      "  color: " + mutedFgHex + ";"
      "  border-color: " + borderHex + ";"
      "}");
}

QString InputDropdown::currentText() const {
  return m_dropdown->currentText();
}

void InputDropdown::setCurrentText(const QString &text) {
  m_dropdown->setCurrentText(text);
}

void InputDropdown::setSize(int width, int height) {
  m_dropdown->setFixedSize(width, height);
}

double InputDropdown::getValue() {
  return m_dropdown->currentIndex();
}

void InputDropdown::setValue() {}
