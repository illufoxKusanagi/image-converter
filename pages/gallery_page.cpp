#include "gallery_page.h"
#include "ui-kit/components/accordion.h"
#include "ui-kit/components/avatar.h"
#include "ui-kit/components/badge.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/separator.h"
#include "ui-kit/components/slider.h"
#include "ui-kit/components/spinner.h"
#include "ui-kit/components/switch.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

GalleryPage::GalleryPage(QWidget *parent)
    : QWidget(parent) {
  setupUi();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this, &GalleryPage::applyThemeStyles);
  applyThemeStyles();
}

void GalleryPage::setupUi() {
  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  m_containerWidget = new QWidget(m_scrollArea);
  m_containerWidget->setObjectName("galleryContainer");

  m_contentLayout = new QVBoxLayout(m_containerWidget);
  m_contentLayout->setContentsMargins(12, 12, 12, 24);
  m_contentLayout->setSpacing(16);

  m_contentLayout->addWidget(createSectionHeader(
      "UI Kit Component Gallery",
      "Interactive showcase of reusable Shadcn-style Qt components"));
  m_contentLayout->addWidget(new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildButtonSection());
  m_contentLayout->addWidget(new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildBadgeSection());
  m_contentLayout->addWidget(new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildFormSection());
  m_contentLayout->addWidget(new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildFeedbackSection());
  m_contentLayout->addWidget(new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildContentSection());

  m_scrollArea->setWidget(m_containerWidget);
  rootLayout->addWidget(m_scrollArea);
}

QWidget *GalleryPage::createSectionHeader(const QString &title, const QString &description) {
  auto *header = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(header);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  const auto &t = ui::Theme::instance().typography();
  const auto &c = ui::Theme::instance().colors();

  auto *titleLabel = new QLabel(title, header);
  titleLabel->setFont(t.font(t.sizeLg, QFont::Bold));
  titleLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                .arg(ui::StyleHelper::toHexString(c.foreground)));
  layout->addWidget(titleLabel);

  if (!description.isEmpty()) {
    auto *descLabel = new QLabel(description, header);
    descLabel->setFont(t.font(t.sizeSm, QFont::Normal));
    descLabel->setStyleSheet(QString("color: %1; background: transparent;")
                                 .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
  }

  return header;
}

QWidget *GalleryPage::buildButtonSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  layout->addWidget(createSectionHeader("Buttons", "All button variants and size scales"));

  // Row 1: Variants
  auto *variantsRow = new QHBoxLayout();
  variantsRow->setSpacing(8);
  variantsRow->addWidget(new ui::Button("Primary", ui::ButtonVariant::Primary, ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Secondary", ui::ButtonVariant::Secondary, ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Outline", ui::ButtonVariant::Outline, ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Ghost", ui::ButtonVariant::Ghost, ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Destructive", ui::ButtonVariant::Destructive, ui::ButtonSize::Default, section));
  variantsRow->addStretch();
  layout->addLayout(variantsRow);

  // Row 2: Sizes & Interactive States
  auto *sizesRow = new QHBoxLayout();
  sizesRow->setSpacing(8);
  sizesRow->addWidget(new ui::Button("Small", ui::ButtonVariant::Primary, ui::ButtonSize::Small, section));
  sizesRow->addWidget(new ui::Button("Default", ui::ButtonVariant::Primary, ui::ButtonSize::Default, section));
  sizesRow->addWidget(new ui::Button("Large", ui::ButtonVariant::Primary, ui::ButtonSize::Large, section));

  m_loadingTestBtn = new ui::Button("Toggle Loading", ui::ButtonVariant::Outline, ui::ButtonSize::Default, section);
  connect(m_loadingTestBtn, &QPushButton::clicked, this, &GalleryPage::onToggleLoadingClicked);
  sizesRow->addWidget(m_loadingTestBtn);
  sizesRow->addStretch();
  layout->addLayout(sizesRow);

  return section;
}

QWidget *GalleryPage::buildBadgeSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(10);

  layout->addWidget(createSectionHeader("Badges", "Status tags and category indicators"));

  auto *row = new QHBoxLayout();
  row->setSpacing(8);
  row->addWidget(new ui::Badge("Default", ui::BadgeVariant::Default, section));
  row->addWidget(new ui::Badge("Secondary", ui::BadgeVariant::Secondary, section));
  row->addWidget(new ui::Badge("Outline", ui::BadgeVariant::Outline, section));
  row->addWidget(new ui::Badge("Destructive", ui::BadgeVariant::Destructive, section));
  row->addStretch();
  layout->addLayout(row);

  return section;
}

QWidget *GalleryPage::buildFormSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(createSectionHeader("Forms & Inputs", "Form controls with labels and validation support"));

  // Input & Select row
  auto *inputRow = new QHBoxLayout();
  inputRow->setSpacing(12);

  auto *inputField = new ui::Input("Enter your username...", section);
  inputField->setClearButtonEnabled(true);
  auto *formField1 = new ui::FormField("Username", inputField, section);
  formField1->setRequired(true);
  formField1->setDescription("Public name displayed in application");
  inputRow->addWidget(formField1);

  auto *selectBox = new ui::Select({"PNG Format", "JPEG Format", "WEBP Format", "PDF Document"}, section);
  auto *formField2 = new ui::FormField("Default Format", selectBox, section);
  formField2->setDescription("Preferred export compression target");
  inputRow->addWidget(formField2);

  layout->addLayout(inputRow);

  // Switch and Slider row
  auto *controlsRow = new QHBoxLayout();
  controlsRow->setSpacing(16);

  auto *switchLayout = new QHBoxLayout();
  switchLayout->setSpacing(8);
  m_switch = new ui::Switch(true, section);
  auto *switchLabel = new QLabel("Enable High Compression", section);
  switchLabel->setStyleSheet(QString("color: %1;").arg(ui::StyleHelper::toHexString(ui::Theme::instance().colors().foreground)));
  switchLayout->addWidget(m_switch);
  switchLayout->addWidget(switchLabel);
  switchLayout->addStretch();
  controlsRow->addLayout(switchLayout, 1);

  auto *sliderLayout = new QVBoxLayout();
  sliderLayout->setSpacing(4);
  auto *sliderLabel = new QLabel("Quality: 65%", section);
  sliderLabel->setStyleSheet(QString("color: %1;").arg(ui::StyleHelper::toHexString(ui::Theme::instance().colors().foreground)));

  m_slider = new ui::Slider(Qt::Horizontal, section);
  m_slider->setRange(0, 100);
  m_slider->setValue(65);

  connect(m_slider, &QSlider::valueChanged, this, [sliderLabel, this](int val) {
    sliderLabel->setText(QString("Quality: %1%").arg(val));
    if (m_progress) {
      m_progress->setValue(val);
    }
  });

  sliderLayout->addWidget(sliderLabel);
  sliderLayout->addWidget(m_slider);
  controlsRow->addLayout(sliderLayout, 1);

  layout->addLayout(controlsRow);

  return section;
}

QWidget *GalleryPage::buildFeedbackSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(createSectionHeader("Indicators & Feedback", "Progress indicators, spinners, and toasts"));

  auto *row = new QHBoxLayout();
  row->setSpacing(16);

  // Spinner
  auto *spinnerLayout = new QHBoxLayout();
  spinnerLayout->setSpacing(8);
  m_spinner = new ui::Spinner(section);
  m_spinner->start();
  spinnerLayout->addWidget(m_spinner);
  auto *spinnerText = new QLabel("Loading task...", section);
  spinnerText->setStyleSheet(QString("color: %1;").arg(ui::StyleHelper::toHexString(ui::Theme::instance().colors().mutedForeground)));
  spinnerLayout->addWidget(spinnerText);
  row->addLayout(spinnerLayout);

  // Progress Bar
  m_progress = new ui::Progress(65, section);
  m_progress->setMinimumWidth(120);
  row->addWidget(m_progress, 1);

  // Toast Triggers
  auto *toastSuccessBtn = new ui::Button("Success Toast", ui::ButtonVariant::Outline, ui::ButtonSize::Small, section);
  connect(toastSuccessBtn, &QPushButton::clicked, this, &GalleryPage::onShowToastSuccessClicked);
  row->addWidget(toastSuccessBtn);

  auto *toastDestructiveBtn = new ui::Button("Error Toast", ui::ButtonVariant::Destructive, ui::ButtonSize::Small, section);
  connect(toastDestructiveBtn, &QPushButton::clicked, this, &GalleryPage::onShowToastDestructiveClicked);
  row->addWidget(toastDestructiveBtn);

  layout->addLayout(row);

  return section;
}

QWidget *GalleryPage::buildContentSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(createSectionHeader("Content & Containers", "Cards, accordions, and avatars"));

  // Card
  auto *card = new ui::Card("Shadcn Design System", "Production-ready Qt 6 component architecture with tokenized styling", section);
  auto *cardBody = new QLabel("All widgets automatically adapt to Light, Dark, and System color schemes via centralized theme tokens.", card);
  cardBody->setWordWrap(true);
  cardBody->setStyleSheet(QString("color: %1;").arg(ui::StyleHelper::toHexString(ui::Theme::instance().colors().mutedForeground)));
  card->contentLayout()->addWidget(cardBody);

  auto *cardActionBtn = new ui::Button("Action", ui::ButtonVariant::Primary, ui::ButtonSize::Small, card);
  card->footerLayout()->addStretch();
  card->footerLayout()->addWidget(cardActionBtn);
  layout->addWidget(card);

  // Accordion
  auto *accordionContent = new QLabel("The accordion container smoothly animates expand and collapse operations without layout jitter.", section);
  accordionContent->setWordWrap(true);
  accordionContent->setStyleSheet(QString("color: %1;").arg(ui::StyleHelper::toHexString(ui::Theme::instance().colors().mutedForeground)));
  auto *accordion = new ui::Accordion("Why use tokenized design?", accordionContent, section);
  layout->addWidget(accordion);

  // Avatars row
  auto *avatarRow = new QHBoxLayout();
  avatarRow->setSpacing(12);
  avatarRow->addWidget(new QLabel("Avatars:", section));
  avatarRow->addWidget(new ui::Avatar("SM", ui::AvatarSize::Small, section));
  avatarRow->addWidget(new ui::Avatar("DF", ui::AvatarSize::Default, section));
  avatarRow->addWidget(new ui::Avatar("LG", ui::AvatarSize::Large, section));
  avatarRow->addStretch();
  layout->addLayout(avatarRow);

  return section;
}

void GalleryPage::onToggleLoadingClicked() {
  if (m_loadingTestBtn) {
    m_loadingTestBtn->setLoading(!m_loadingTestBtn->isLoading());
  }
}

void GalleryPage::onShowToastSuccessClicked() {
  ui::Toast::show(this, "Operation Completed", "All components rendered and verified successfully.", ui::ToastVariant::Success);
}

void GalleryPage::onShowToastDestructiveClicked() {
  ui::Toast::show(this, "Operation Failed", "Could not complete the simulated action.", ui::ToastVariant::Destructive);
}

void GalleryPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  QString bgHex = ui::StyleHelper::toHexString(c.background);

  setStyleSheet(QString("QScrollArea { background-color: transparent; border: none; }"
                        "QWidget#galleryContainer { background-color: %1; }")
                    .arg(bgHex));
}
