#include "mainwindow.h"
#include "ui-kit/theme/style_helper.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("ImageConverter");
  setMinimumSize(540, 680);

  m_centralWidget = new QWidget(this);
  QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);
  rootLayout->setContentsMargins(24, 16, 24, 24);
  rootLayout->setSpacing(16);

  // Top navigation & utility header
  QWidget *headerWidget = new QWidget(m_centralWidget);
  QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(0, 0, 0, 0);
  headerLayout->setSpacing(12);

  m_titleLabel = new QLabel("ImageConverter", headerWidget);

  m_themeToggleBtn = new ui::Button(ui::Theme::instance().isDark() ? "☀️ Light" : "🌙 Dark",
                                    ui::ButtonVariant::Ghost,
                                    ui::ButtonSize::Small,
                                    headerWidget);
  connect(m_themeToggleBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggleClicked);

  headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
  headerLayout->addStretch(1);
  headerLayout->addWidget(m_themeToggleBtn, 0, Qt::AlignVCenter);
  rootLayout->addWidget(headerWidget);

  // Main Tabs
  m_tabs = new ui::Tabs(ui::TabVariant::Underline, m_centralWidget);
  m_mainPage = new MainPage(m_tabs);
  m_tabs->addTab("Convert Image", QIcon(), m_mainPage);

  rootLayout->addWidget(m_tabs, 1);
  setCentralWidget(m_centralWidget);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this, &MainWindow::applyThemeStyles);
  applyThemeStyles();
}

void MainWindow::onThemeToggleClicked() {
  ui::Theme::instance().toggleMode();
}

void MainWindow::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();
  bool isDark = ui::Theme::instance().isDark();

  setStyleSheet(QString("QMainWindow { background-color: %1; }")
                    .arg(ui::StyleHelper::toHexString(c.background)));

  m_centralWidget->setStyleSheet(QString("background-color: %1;")
                                     .arg(ui::StyleHelper::toHexString(c.background)));

  m_titleLabel->setFont(t.font(t.sizeLg, QFont::Bold));
  m_titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                  .arg(ui::StyleHelper::toHexString(c.foreground)));

  m_themeToggleBtn->setText(isDark ? "☀️ Light" : "🌙 Dark");
}