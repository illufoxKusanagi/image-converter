#include "mainwindow.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setMinimumSize(460, 620);
  setWindowTitle("ImageConverter");

  m_centralWidget = new QWidget(this);
  QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);
  rootLayout->setContentsMargins(20, 18, 20, 20);
  rootLayout->setSpacing(18);

  // Header: Title & Theme Toggle
  QWidget *headerWidget = new QWidget(m_centralWidget);
  QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(0, 0, 0, 0);

  m_titleLabel = new QLabel("ImageConverter", headerWidget);
  m_titleLabel->setFont(ui::Theme::instance().typography().font(
      ui::Theme::instance().typography().sizeXl, QFont::Bold));

  m_themeToggleBtn = new ui::Button(
      ui::Theme::instance().isDark() ? "Light" : "Dark",
      ui::ButtonVariant::Ghost, ui::ButtonSize::Small, headerWidget);
  connect(m_themeToggleBtn, &QPushButton::clicked, this,
          &MainWindow::onThemeToggleClicked);

  headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
  headerLayout->addStretch(1);
  headerLayout->addWidget(m_themeToggleBtn, 0, Qt::AlignVCenter);
  rootLayout->addWidget(headerWidget);

  // Main Tabs
  m_tabs = new ui::Tabs(ui::TabVariant::Underline, m_centralWidget);
  m_mainPage = new MainPage(m_tabs);
  m_tabs->addTab("Convert Image",
                 ui::Icon::get(":/icons/icons/lucide-image.svg",
                               ui::Theme::instance().colors().foreground,
                               QSize(16, 16)),
                 m_mainPage);

  m_galleryPage = new GalleryPage(m_tabs);
  m_tabs->addTab("UI Components",
                 ui::Icon::get(":/icons/icons/lucide-sliders.svg",
                               ui::Theme::instance().colors().foreground,
                               QSize(16, 16)),
                 m_galleryPage);

  rootLayout->addWidget(m_tabs, 1);
  setCentralWidget(m_centralWidget);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainWindow::applyThemeStyles);
  applyThemeStyles();
}

void MainWindow::onThemeToggleClicked() { ui::Theme::instance().toggleMode(); }

void MainWindow::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  ui::ThemeMode mode = ui::Theme::instance().mode();

  setStyleSheet(QString("QMainWindow { background-color: %1; }")
                    .arg(ui::StyleHelper::toHexString(c.background)));

  m_centralWidget->setStyleSheet(
      QString("background-color: %1;")
          .arg(ui::StyleHelper::toHexString(c.background)));

  m_titleLabel->setStyleSheet(
      QString("color: %1;").arg(ui::StyleHelper::toHexString(c.foreground)));

  QString modeText;
  QString iconPath;
  if (mode == ui::ThemeMode::Light) {
    modeText = "Light";
    iconPath = ":/icons/icons/lucide-sun.svg";
  } else if (mode == ui::ThemeMode::Dark) {
    modeText = "Dark";
    iconPath = ":/icons/icons/lucide-moon.svg";
  } else {
    modeText = "System";
    iconPath = ":/icons/icons/lucide-monitor.svg";
  }

  m_themeToggleBtn->setText(modeText);
  m_themeToggleBtn->setIcon(
      ui::Icon::get(iconPath, c.foreground, QSize(16, 16)));
}