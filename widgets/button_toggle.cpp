#include "button_toggle.h"

/**
 * @brief Constructs a toggle button with a specified collapsed state and parent widget.
 *
 * Initializes the button's icon, size, margins, and visual style based on the collapsed state.
 *
 * @param parent The parent widget.
 * @param isCollapsed Whether the button starts in the collapsed state.
 */
ButtonToggle::ButtonToggle(QWidget *parent, bool isCollapsed)
    : QPushButton(parent) {
  updateIcon();
  setIconSize(QSize(36, 36));
  setFixedSize(64, 56);
  setContentsMargins(16, 16, 16, 16);
  setStyleSheet("QPushButton {"
                "    padding: 32px;"
                "    background-color: transparent;"
                "    border: none;"
                "    border-radius: 8px;"
                "}"
                "QPushButton:hover {"
                "    background-color: " +
                Colors::Grey400.name() +
                ";"
                "}");
}

void ButtonToggle::updateIcon() {
  QString iconPath =
      m_isCollapsed ? ":/icons/icons/expand.svg" : ":/icons/icons/collapse.svg";
  setIcon(QIcon(iconPath));
}

void ButtonToggle::toggleCollapse() {
  m_isCollapsed = !m_isCollapsed;
  updateIcon();
}
