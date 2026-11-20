#include "mainwindow.h"
#include "components/button.h"
#include "mainwindow/trakteer_support_button.h"
#include "mainwindow/update_checker.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"

namespace {
// Placeholder support URL — replace with final Ko-fi link.
// constexpr const char *kKofiUrl = "https://trakteer.id/indah_sinara/tipid";
constexpr int kHeaderIconButtonSize = 32;
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setMinimumSize(460, 620);
  setWindowTitle(tr("ImageConverter"));

  m_centralWidget = new QWidget(this);
  m_centralWidget->setObjectName(QStringLiteral("centralWidget"));
  QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);
  rootLayout->setContentsMargins(20, 18, 20, 20);
  rootLayout->setSpacing(18);

  // Header: Title, Support Links, Update Check, Language Toggle & Theme Toggle
  QWidget *headerWidget = new QWidget(m_centralWidget);
  QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
  headerLayout->setContentsMargins(0, 0, 0, 0);

  m_titleLabel = new QLabel(tr("ImageConverter"), headerWidget);
  m_titleLabel->setFont(ui::Theme::instance().typography().font(
      ui::Theme::instance().typography().sizeXl, QFont::Bold));

  m_trakteerBtn = new TrakteerSupportButton(m_centralWidget, headerWidget);

  // m_kofiBtn = new ui::Button("", ui::ButtonVariant::Ghost,
  // ui::ButtonSize::Icon,
  //                            headerWidget);
  // m_kofiBtn->setIconSize(QSize(16, 16));
  // connect(m_kofiBtn, &QPushButton::clicked, this,
  // &MainWindow::onKofiClicked);

  m_updateChecker = new UpdateChecker(this, this);
  m_updateCheckBtn = new ui::Button("", ui::ButtonVariant::Ghost,
                                    ui::ButtonSize::Icon, headerWidget);
  m_updateCheckBtn->setIconSize(QSize(16, 16));
  connect(m_updateCheckBtn, &QPushButton::clicked, m_updateChecker,
          &UpdateChecker::checkManually);
  connect(m_updateChecker, &UpdateChecker::checkingChanged, m_updateCheckBtn,
          [this](bool checking) {
            if (m_updateCheckBtn) {
              m_updateCheckBtn->setEnabled(!checking);
            }
          });

  m_langToggleBtn = new ui::Button(ui::I18n::instance().displayBadge(),
                                   ui::ButtonVariant::Ghost,
                                   ui::ButtonSize::Small, headerWidget);
  connect(m_langToggleBtn, &QPushButton::clicked, this,
          &MainWindow::onLanguageToggleClicked);

  m_themeToggleBtn = new ui::Button(
      ui::Theme::instance().isDark() ? tr("Light") : tr("Dark"),
      ui::ButtonVariant::Ghost, ui::ButtonSize::Small, headerWidget);
  connect(m_themeToggleBtn, &QPushButton::clicked, this,
          &MainWindow::onThemeToggleClicked);

  headerLayout->addWidget(m_titleLabel, 0, Qt::AlignVCenter);
  headerLayout->addStretch(1);
  headerLayout->addWidget(m_trakteerBtn, 0, Qt::AlignVCenter);
  // headerLayout->addWidget(m_kofiBtn, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_updateCheckBtn, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_langToggleBtn, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_themeToggleBtn, 0, Qt::AlignVCenter);
  rootLayout->addWidget(headerWidget);

  // Main Tabs: Convert Image, Compress PDF, UI Components
  m_tabs = new ui::Tabs(ui::TabVariant::Underline, m_centralWidget);

  m_mainPage = new MainPage(m_tabs);
  m_tabs->addTab(tr("Convert Image"), QString(":/icons/icons/lucide-image.svg"),
                 m_mainPage);

  m_pdfPage = new PdfPage(m_tabs);
  m_tabs->addTab(tr("PDF Tools"), QString(":/icons/icons/lucide-file-text.svg"),
                 m_pdfPage);

  // m_galleryPage = new GalleryPage(m_tabs);
  // m_tabs->addTab("UI Components",
  // QString(":/icons/icons/lucide-sliders.svg"),
  //                m_galleryPage);

  rootLayout->addWidget(m_tabs, 1);
  setCentralWidget(m_centralWidget);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainWindow::applyThemeStyles);
  connect(&ui::I18n::instance(), &ui::I18n::languageChanged, this,
          &MainWindow::retranslateUi);
  retranslateUi();
  m_updateChecker->checkIfWeeklyIntervalElapsed();
}

void MainWindow::onThemeToggleClicked() { ui::Theme::instance().toggleMode(); }

void MainWindow::onLanguageToggleClicked() {
  ui::I18n::instance().toggleLanguage();
}

// void MainWindow::onKofiClicked() {
//   QDesktopServices::openUrl(QUrl(QString::fromLatin1(kKofiUrl)));
// }

void MainWindow::retranslateUi() {
  setWindowTitle(tr("ImageConverter"));
  if (m_titleLabel) {
    m_titleLabel->setText(tr("ImageConverter"));
  }
  if (m_tabs) {
    m_tabs->setTabText(0, tr("Convert Image"));
    m_tabs->setTabText(1, tr("PDF Tools"));
  }
  // if (m_kofiBtn) {
  //   const QString label = tr("Support on Ko-fi");
  //   m_kofiBtn->setToolTip(label);
  //   m_kofiBtn->setAccessibleName(label);
  // }
  if (m_updateCheckBtn) {
    const QString label = tr("Check for Updates");
    m_updateCheckBtn->setToolTip(label);
    m_updateCheckBtn->setAccessibleName(label);
  }
  if (m_langToggleBtn) {
    m_langToggleBtn->setText(ui::I18n::instance().displayBadge());
    m_langToggleBtn->setToolTip(
        tr("Switch Language (English / Bahasa Indonesia)"));
  }
  applyThemeStyles();
}

void MainWindow::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  ui::ThemeMode mode = ui::Theme::instance().mode();

  setStyleSheet(QString("QMainWindow { background-color: %1; }")
                    .arg(ui::StyleHelper::toHexString(c.background)));

  m_centralWidget->setStyleSheet(
      QString("#centralWidget { background-color: %1; }")
          .arg(ui::StyleHelper::toHexString(c.background)));

  m_titleLabel->setStyleSheet(
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground)));

  // if (m_kofiBtn) {
  //   m_kofiBtn->setFixedSize(kHeaderIconButtonSize, kHeaderIconButtonSize);
  //   m_kofiBtn->setIcon(ui::Icon::get(":/icons/icons/lucide-coffee.svg",
  //                                    c.foreground, QSize(16, 16)));
  // }
  if (m_updateCheckBtn) {
    m_updateCheckBtn->setSize(ui::ButtonSize::Default);
    m_updateCheckBtn->setIcon(ui::Icon::get(
        ":/icons/icons/lucide-refresh-cw.svg", c.foreground, QSize(16, 16)));
    m_updateCheckBtn->setText(tr("Check Update"));
  }

  QString modeText;
  QString iconPath;
  if (mode == ui::ThemeMode::Light) {
    modeText = tr("Light");
    iconPath = ":/icons/icons/lucide-sun.svg";
  } else if (mode == ui::ThemeMode::Dark) {
    modeText = tr("Dark");
    iconPath = ":/icons/icons/lucide-moon.svg";
  } else {
    modeText = tr("System");
    iconPath = ":/icons/icons/lucide-monitor.svg";
  }

  m_themeToggleBtn->setText(modeText);
  m_themeToggleBtn->setIcon(
      ui::Icon::get(iconPath, c.foreground, QSize(16, 16)));
}