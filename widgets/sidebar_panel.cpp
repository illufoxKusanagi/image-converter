#include "sidebar_panel.h"

SidebarPanel::SidebarPanel(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_buttonLayout(nullptr) {
  mainLayout->setContentsMargins(0, 0, 0, 0);
  m_buttonToggle = new ButtonToggle(m_isCollapsed, this);
  QWidget *buttonContainer = new QWidget(this);
  m_buttonLayout = new QVBoxLayout(buttonContainer);
  m_buttonLayout->setContentsMargins(8, 8, 8, 8);
  m_buttonLayout->setSpacing(8);
  m_buttonLayout->setAlignment(Qt::AlignTop);
  m_buttonLayout->addWidget(m_buttonToggle);
  mainLayout->addWidget(buttonContainer);
  connect(m_buttonToggle, &QPushButton::clicked, this, [this]() {
    m_isCollapsed = !m_isCollapsed;
    m_buttonToggle->toggleCollapse();
    setFixedWidth(m_isCollapsed ? 160 : 320);
  });
  createSidebarButtons();
  setFixedWidth(320);
  setLayout(mainLayout);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  setStyleSheet("QWidget {"
                "    background-color: " +
                Colors::Secondary600.name() +
                ";"
                "}"
                // "QScrollArea {"
                // "    background-color: " +
                // Colors::Secondary600.name() +
                // ";"
                // "}"
  );
}

void SidebarPanel::emitNavigateSignal(int pageIndex) {
  emit navigateToPage(pageIndex);
}

void SidebarPanel::createSidebarButtons() {
  QStringList buttonNames = {"Convert and Compress Image", "Compress PDF"};
  QStringList buttonIcons = {"output", "output"};
  for (int i = 0; i < buttonNames.size(); i++) {
    ButtonSidebarActive *button =
        new ButtonSidebarActive(buttonIcons[i], buttonNames[i], this);
    button->setIconSize(40, 40);
    button->setEnabled(true);
    connect(button, &QPushButton::clicked, this, [this, i]() {
      m_currentIndex = i;
      // updateButtonStates();
      emit buttonClicked(m_currentIndex);
    });
    m_categoryButtons.append(button);
    m_buttonLayout->addWidget(button);
  }
}

void SidebarPanel::updateButtonState() {
  for (int i = 0; i < m_categoryButtons.size(); i++) {
    m_categoryButtons[i]->setEnabled(i == m_currentIndex);
  }
}

void SidebarPanel::onPageChanged(int pageIndex) {
  m_currentIndex = pageIndex;
  updateButtonState();
}

void SidebarPanel::updateButtonState(QFuture<void> future,
                                     ButtonAction *runButton,
                                     ButtonAction *runStaticButton) {}
