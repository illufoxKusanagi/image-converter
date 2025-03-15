#include "left_panel.h"

LeftPanel::LeftPanel(QWidget *parent)
    : QWidget(parent), m_buttonLayout(nullptr) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  m_buttonToggle = new ButtonToggle(m_isCollapsed, this);
  QWidget *buttonContainer = new QWidget(this);
  m_buttonLayout = new QVBoxLayout(buttonContainer);
  m_buttonLayout->setContentsMargins(8, 8, 8, 8);
  m_buttonLayout->setSpacing(8);
  m_buttonLayout->setAlignment(Qt::AlignTop);
  m_buttonLayout->addWidget(m_buttonToggle);

  connect(m_buttonToggle, &QPushButton::clicked, this, [this]() {
    m_isCollapsed = !m_isCollapsed;
    m_buttonToggle->toggleCollapse();
    m_runButtonLayout->setContentsMargins(0, 0, 0, 0);
    setFixedWidth(m_isCollapsed ? 160 : 320);
  });
  setupInputPageButtons();
  createRunButton();
  setupOutputPageButtons();
  QScrollArea *scrollArea = new QScrollArea(this);
  scrollArea->setWidget(buttonContainer);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setFrameShape(QFrame::NoFrame);
  mainLayout->addWidget(scrollArea);
  setFixedWidth(320);
  setLayout(mainLayout);
  setStyleSheet("QWidget {"
                "    background-color: " +
                Colors::Secondary500.name() +
                ";"
                "}"
                "QScrollArea {"
                "    background-color: " +
                Colors::Secondary500.name() +
                ";"
                "}");
}

void LeftPanel::emitNavigateSignal(int pageIndex) {
  emit navigateToPage(pageIndex);
}

void LeftPanel::createRunButton() {}

void LeftPanel::setupInputPageButtons() {}

void LeftPanel::setupOutputPageButtons() {}

void LeftPanel::updateButtonState(QFuture<void> future, ButtonAction *runButton,
                                  ButtonAction *runStaticButton) {}
