#include "../components/badge.h"
#include "../components/button.h"
#include "../components/separator.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include "demo_page.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  ui::Theme::instance().setMode(ui::ThemeMode::System);

  QMainWindow window;
  window.setWindowTitle(QStringLiteral("Qt UI-Kit Showcase"));
  window.resize(960, 720);
  window.setMinimumSize(520, 620);

  auto *central = new QWidget(&window);
  central->setObjectName(QStringLiteral("demoRoot"));
  central->setAttribute(Qt::WA_StyledBackground, true);

  auto *rootLayout = new QVBoxLayout(central);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  auto *headerBar = new QWidget(central);
  headerBar->setObjectName(QStringLiteral("demoHeaderBar"));
  headerBar->setAttribute(Qt::WA_StyledBackground, true);

  auto *headerLayout = new QHBoxLayout(headerBar);
  headerLayout->setContentsMargins(16, 12, 16, 12);
  headerLayout->setSpacing(12);

  auto *titleLabel = new QLabel(QStringLiteral("Qt UI-Kit Showcase"), headerBar);
  headerLayout->addWidget(titleLabel);

  auto *versionBadge =
      new ui::Badge(QStringLiteral("30 Components"), ui::BadgeVariant::Secondary, headerBar);
  headerLayout->addWidget(versionBadge);
  headerLayout->addStretch();

  auto *themeToggleBtn = new ui::Button(
      QStringLiteral("Theme: System"), ui::ButtonVariant::Outline,
      ui::ButtonSize::Small, headerBar);
  headerLayout->addWidget(themeToggleBtn);

  rootLayout->addWidget(headerBar);
  rootLayout->addWidget(new ui::Separator(Qt::Horizontal, central));
  rootLayout->addWidget(new ui::DemoPage(central), 1);

  auto applyShellTheme = [central, headerBar, titleLabel, themeToggleBtn]() {
    const auto &theme = ui::Theme::instance();
    const auto &c = theme.colors();
    const auto &t = theme.typography();

    central->setStyleSheet(
        QStringLiteral("QWidget#demoRoot { background-color: %1; }")
            .arg(ui::StyleHelper::toHexString(c.background)));
    headerBar->setStyleSheet(
        QStringLiteral("QWidget#demoHeaderBar { background-color: %1; }")
            .arg(ui::StyleHelper::toHexString(c.card)));

    titleLabel->setFont(t.font(t.sizeLg, QFont::Bold));
    titleLabel->setStyleSheet(
        QStringLiteral("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.foreground)));

    const QString modeText =
        theme.isDark() ? QStringLiteral("Theme: Dark (Switch to Light)")
                       : QStringLiteral("Theme: Light (Switch to Dark)");
    themeToggleBtn->setText(modeText);
  };

  QObject::connect(themeToggleBtn, &QPushButton::clicked, &window, []() {
    ui::Theme::instance().toggleMode();
  });
  QObject::connect(&ui::Theme::instance(), &ui::Theme::themeChanged, &window,
                   [applyShellTheme](ui::ThemeMode) { applyShellTheme(); });

  applyShellTheme();

  window.setCentralWidget(central);
  window.show();

  return app.exec();
}
