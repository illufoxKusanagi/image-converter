#include "gallery_page.h"
#include "ui-kit/components/accordion.h"
#include "ui-kit/components/alert.h"
#include "ui-kit/components/avatar.h"
#include "ui-kit/components/badge.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/card.h"
#include "ui-kit/components/checkbox.h"
#include "ui-kit/components/combobox.h"
#include "ui-kit/components/empty_state.h"
#include "ui-kit/components/form_field.h"
#include "ui-kit/components/input.h"
#include "ui-kit/components/menu.h"
#include "ui-kit/components/number_input.h"
#include "ui-kit/components/popover.h"
#include "ui-kit/components/progress.h"
#include "ui-kit/components/radio_group.h"
#include "ui-kit/components/scroll_area.h"
#include "ui-kit/components/select.h"
#include "ui-kit/components/separator.h"
#include "ui-kit/components/slider.h"
#include "ui-kit/components/spinner.h"
#include "ui-kit/components/splitter.h"
#include "ui-kit/components/switch.h"
#include "ui-kit/components/textarea.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"

#include <QAction>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

namespace {

QLabel *galleryLabel(const QString &text, QWidget *parent,
                     const char *role = "mutedText") {
  auto *label = new QLabel(text, parent);
  label->setTextFormat(Qt::PlainText);
  label->setWordWrap(true);
  label->setProperty("galleryRole", role);
  return label;
}

QWidget *controlExample(const QString &title, const QString &description,
                        QWidget *control, QWidget *parent) {
  auto *example = new QWidget(parent);
  auto *layout = new QVBoxLayout(example);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(ui::Theme::instance().spacing().space1);
  auto *label = galleryLabel(title, example, "bodyText");
  label->setBuddy(control);
  control->setAccessibleName(title);
  control->setAccessibleDescription(description);
  layout->addWidget(label);
  layout->addWidget(control);
  if (!description.isEmpty()) {
    layout->addWidget(galleryLabel(description, example));
  }
  return example;
}

} // namespace

GalleryPage::GalleryPage(QWidget *parent) : QWidget(parent) {
  setupUi();
  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &GalleryPage::applyThemeStyles);
  applyThemeStyles();
}

void GalleryPage::setupUi() {
  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  m_scrollArea = new ui::ScrollArea(this);
  m_scrollArea->setObjectName("galleryScrollArea");
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
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildButtonSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildBadgeSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildFormSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildFeedbackSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));

  m_contentLayout->addWidget(buildContentSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));
  m_contentLayout->addWidget(buildSelectionSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));
  m_contentLayout->addWidget(buildTextNumberSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));
  m_contentLayout->addWidget(buildActionsSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));
  m_contentLayout->addWidget(buildStatusSection());
  m_contentLayout->addWidget(
      new ui::Separator(Qt::Horizontal, m_containerWidget));
  m_contentLayout->addWidget(buildLayoutSection());

  m_scrollArea->setWidget(m_containerWidget);
  rootLayout->addWidget(m_scrollArea);
}

QWidget *GalleryPage::createSectionHeader(const QString &title,
                                          const QString &description) {
  auto *header = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(header);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  const auto &t = ui::Theme::instance().typography();
  const auto &c = ui::Theme::instance().colors();

  auto *titleLabel = new QLabel(title, header);
  titleLabel->setProperty("galleryRole", "sectionTitle");
  titleLabel->setFont(t.font(t.sizeLg, QFont::Bold));
  titleLabel->setStyleSheet(
      QString("color: %1; background: transparent;")
          .arg(ui::StyleHelper::toHexString(c.foreground)));
  layout->addWidget(titleLabel);

  if (!description.isEmpty()) {
    auto *descLabel = new QLabel(description, header);
    descLabel->setProperty("galleryRole", "mutedText");
    descLabel->setFont(t.font(t.sizeSm, QFont::Normal));
    descLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
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

  layout->addWidget(
      createSectionHeader("Buttons", "All button variants and size scales"));

  // Row 1: Variants
  auto *variantsRow = new QHBoxLayout();
  variantsRow->setSpacing(8);
  variantsRow->addWidget(new ui::Button("Primary", ui::ButtonVariant::Primary,
                                        ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Secondary",
                                        ui::ButtonVariant::Secondary,
                                        ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Outline", ui::ButtonVariant::Outline,
                                        ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Ghost", ui::ButtonVariant::Ghost,
                                        ui::ButtonSize::Default, section));
  variantsRow->addWidget(new ui::Button("Destructive",
                                        ui::ButtonVariant::Destructive,
                                        ui::ButtonSize::Default, section));
  variantsRow->addStretch();
  layout->addLayout(variantsRow);

  // Row 2: Sizes & Interactive States
  auto *sizesRow = new QHBoxLayout();
  sizesRow->setSpacing(8);
  sizesRow->addWidget(new ui::Button("Small", ui::ButtonVariant::Primary,
                                     ui::ButtonSize::Small, section));
  sizesRow->addWidget(new ui::Button("Default", ui::ButtonVariant::Primary,
                                     ui::ButtonSize::Default, section));
  sizesRow->addWidget(new ui::Button("Large", ui::ButtonVariant::Primary,
                                     ui::ButtonSize::Large, section));

  m_loadingTestBtn =
      new ui::Button("Toggle Loading", ui::ButtonVariant::Outline,
                     ui::ButtonSize::Default, section);
  connect(m_loadingTestBtn, &QPushButton::clicked, this,
          &GalleryPage::onToggleLoadingClicked);
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

  layout->addWidget(
      createSectionHeader("Badges", "Status tags and category indicators"));

  auto *row = new QHBoxLayout();
  row->setSpacing(8);
  row->addWidget(new ui::Badge("Default", ui::BadgeVariant::Default, section));
  row->addWidget(
      new ui::Badge("Secondary", ui::BadgeVariant::Secondary, section));
  row->addWidget(new ui::Badge("Outline", ui::BadgeVariant::Outline, section));
  row->addWidget(
      new ui::Badge("Destructive", ui::BadgeVariant::Destructive, section));
  row->addStretch();
  layout->addLayout(row);

  return section;
}

QWidget *GalleryPage::buildFormSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(createSectionHeader(
      "Forms & Inputs", "Form controls with labels and validation support"));

  // Input & Select row
  auto *inputRow = new QHBoxLayout();
  inputRow->setSpacing(12);

  auto *inputField = new ui::Input("Enter your username...", section);
  inputField->setClearButtonEnabled(true);
  auto *formField1 = new ui::FormField("Username", inputField, section);
  formField1->setRequired(true);
  formField1->setDescription("Public name displayed in application");
  inputRow->addWidget(formField1);

  auto *selectBox = new ui::Select(
      {"PNG Format", "JPEG Format", "WEBP Format", "PDF Document"}, section);
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
  switchLabel->setProperty("galleryRole", "bodyText");
  switchLabel->setStyleSheet(
      QString("color: %1;")
          .arg(ui::StyleHelper::toHexString(
              ui::Theme::instance().colors().foreground)));
  switchLayout->addWidget(m_switch);
  switchLayout->addWidget(switchLabel);
  switchLayout->addStretch();
  controlsRow->addLayout(switchLayout, 1);

  auto *sliderLayout = new QVBoxLayout();
  sliderLayout->setSpacing(4);
  auto *sliderLabel = new QLabel("Quality: 65%", section);
  sliderLabel->setProperty("galleryRole", "bodyText");
  sliderLabel->setStyleSheet(
      QString("color: %1;")
          .arg(ui::StyleHelper::toHexString(
              ui::Theme::instance().colors().foreground)));

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

  layout->addWidget(createSectionHeader(
      "Indicators & Feedback", "Progress indicators, spinners, and toasts"));

  auto *row = new QHBoxLayout();
  row->setSpacing(16);

  // Spinner
  auto *spinnerLayout = new QHBoxLayout();
  spinnerLayout->setSpacing(8);
  m_spinner = new ui::Spinner(section);
  m_spinner->start();
  spinnerLayout->addWidget(m_spinner);
  auto *spinnerText = new QLabel("Loading task...", section);
  spinnerText->setProperty("galleryRole", "mutedText");
  spinnerText->setStyleSheet(
      QString("color: %1;")
          .arg(ui::StyleHelper::toHexString(
              ui::Theme::instance().colors().mutedForeground)));
  spinnerLayout->addWidget(spinnerText);
  row->addLayout(spinnerLayout);

  // Progress Bar
  m_progress = new ui::Progress(65, section);
  m_progress->setMinimumWidth(120);
  row->addWidget(m_progress, 1);

  // Toast Triggers
  auto *toastSuccessBtn =
      new ui::Button("Success Toast", ui::ButtonVariant::Outline,
                     ui::ButtonSize::Small, section);
  connect(toastSuccessBtn, &QPushButton::clicked, this,
          &GalleryPage::onShowToastSuccessClicked);
  row->addWidget(toastSuccessBtn);

  auto *toastDestructiveBtn =
      new ui::Button("Error Toast", ui::ButtonVariant::Destructive,
                     ui::ButtonSize::Small, section);
  connect(toastDestructiveBtn, &QPushButton::clicked, this,
          &GalleryPage::onShowToastDestructiveClicked);
  row->addWidget(toastDestructiveBtn);

  layout->addLayout(row);

  return section;
}

QWidget *GalleryPage::buildContentSection() {
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(createSectionHeader("Content & Containers",
                                        "Cards, accordions, and avatars"));

  // Card
  auto *card = new ui::Card(
      "Shadcn Design System",
      "Production-ready Qt 6 component architecture with tokenized styling",
      section);
  auto *cardBody =
      new QLabel("All widgets automatically adapt to Light, Dark, and System "
                 "color schemes via centralized theme tokens.",
                 card);
  cardBody->setProperty("galleryRole", "mutedText");
  cardBody->setWordWrap(true);
  cardBody->setStyleSheet(
      QString("color: %1;")
          .arg(ui::StyleHelper::toHexString(
              ui::Theme::instance().colors().mutedForeground)));
  card->contentLayout()->addWidget(cardBody);

  auto *cardActionBtn = new ui::Button("Action", ui::ButtonVariant::Primary,
                                       ui::ButtonSize::Small, card);
  card->footerLayout()->addStretch();
  card->footerLayout()->addWidget(cardActionBtn);
  layout->addWidget(card);

  // Accordion
  auto *accordionContent =
      new QLabel("The accordion container smoothly animates expand and "
                 "collapse operations without layout jitter.",
                 section);
  accordionContent->setProperty("galleryRole", "mutedText");
  accordionContent->setWordWrap(true);
  accordionContent->setStyleSheet(
      QString("color: %1;")
          .arg(ui::StyleHelper::toHexString(
              ui::Theme::instance().colors().mutedForeground)));
  auto *accordion =
      new ui::Accordion("Why use tokenized design?", accordionContent, section);
  layout->addWidget(accordion);

  // Avatars row
  auto *avatarRow = new QHBoxLayout();
  avatarRow->setSpacing(12);
  auto *avatarLabel = new QLabel("Avatars:", section);
  avatarLabel->setProperty("galleryRole", "bodyText");
  avatarRow->addWidget(avatarLabel);
  avatarRow->addWidget(new ui::Avatar("SM", ui::AvatarSize::Small, section));
  avatarRow->addWidget(new ui::Avatar("DF", ui::AvatarSize::Default, section));
  avatarRow->addWidget(new ui::Avatar("LG", ui::AvatarSize::Large, section));
  avatarRow->addStretch();
  layout->addLayout(avatarRow);

  return section;
}

QWidget *GalleryPage::buildSelectionSection() {
  const auto &s = ui::Theme::instance().spacing();
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(s.space3);
  layout->addWidget(
      createSectionHeader(tr("Checkbox, RadioGroup & Combobox"),
                          tr("Native choices and searchable selection; these "
                             "examples do not change conversion settings.")));

  auto *choices = new QGridLayout();
  choices->setSpacing(s.space4);
  choices->setColumnStretch(0, 1);
  choices->setColumnStretch(1, 1);
  auto *checkboxes = new QWidget(section);
  auto *checkboxLayout = new QVBoxLayout(checkboxes);
  checkboxLayout->setContentsMargins(0, 0, 0, 0);
  checkboxLayout->setSpacing(s.space2);
  checkboxLayout->addWidget(
      new ui::Checkbox(tr("Unchecked choice"), checkboxes));
  auto *checked = new ui::Checkbox(tr("Checked choice"), checkboxes);
  checked->setChecked(true);
  checkboxLayout->addWidget(checked);
  auto *partial = new ui::Checkbox(tr("Tri-state choice"), checkboxes);
  partial->setTristate(true);
  partial->setCheckState(Qt::PartiallyChecked);
  checkboxLayout->addWidget(partial);
  auto *disabled = new ui::Checkbox(tr("Disabled choice"), checkboxes);
  disabled->setChecked(true);
  disabled->setEnabled(false);
  checkboxLayout->addWidget(disabled);
  auto *required = new ui::Checkbox(
      tr("Required choice (check to clear the error)"), checkboxes);
  required->setError(true);
  required->setAccessibleDescription(
      tr("This example requires the choice to be checked."));
  connect(required, &QCheckBox::toggled, this,
          [required](bool selected) { required->setError(!selected); });
  checkboxLayout->addWidget(required);
  choices->addWidget(checkboxes, 0, 0);

  auto *radios = new ui::RadioGroup(
      {tr("Standard"), tr("Balanced"), tr("Maximum"), tr("Unavailable")},
      section);
  radios->setCheckedId(1);
  radios->button(3)->setEnabled(false);
  auto *radioExample = controlExample(
      tr("Exclusive choice"),
      tr("Use arrow keys to change the selected option."), radios, section);
  auto *radioStatus =
      galleryLabel(tr("Selected option: Balanced"), radioExample);
  radioExample->layout()->addWidget(radioStatus);
  connect(radios, &ui::RadioGroup::selectionChanged, this,
          [radios, radioStatus](int id) {
            auto *button = radios->button(id);
            radioStatus->setText(
                button ? tr("Selected option: %1").arg(button->text())
                       : tr("No option selected"));
          });
  choices->addWidget(radioExample, 0, 1, Qt::AlignTop);
  layout->addLayout(choices);

  auto *combos = new QGridLayout();
  combos->setSpacing(s.space3);
  combos->setColumnStretch(0, 1);
  combos->setColumnStretch(1, 1);
  auto *search = new ui::Combobox(section);
  search->addItem(tr("PNG"), QStringLiteral("lossless"));
  search->addItem(tr("JPEG"), QStringLiteral("photographic"));
  search->addItem(tr("WEBP"), QStringLiteral("web"));
  search->addItem(tr("TIFF"), QStringLiteral("archival"));
  search->addItem(tr("PNG"), QStringLiteral("indexed"));
  search->setPlaceholderText(tr("Search formats..."));
  search->setCurrentIndex(1);
  auto *searchExample =
      controlExample(tr("Searchable format"),
                     tr("Type to filter. Enter selects; Escape cancels. "
                        "Duplicate labels keep separate data."),
                     search, section);
  auto *searchStatus = galleryLabel(
      tr("Selected: JPEG — photographic (source index 1)"), searchExample);
  searchExample->layout()->addWidget(searchStatus);
  connect(search, &ui::Combobox::currentIndexChanged, this,
          [search, searchStatus](int index) {
            searchStatus->setText(
                index < 0 ? tr("No format selected")
                          : tr("Selected: %1 — %2 (source index %3)")
                                .arg(search->currentText(),
                                     search->currentData().toString())
                                .arg(index));
          });
  combos->addWidget(searchExample, 0, 0);
  auto *disabledCombo =
      new ui::Combobox({tr("Unavailable selection")}, section);
  disabledCombo->setCurrentIndex(0);
  disabledCombo->setEnabled(false);
  combos->addWidget(controlExample(tr("Disabled combobox"),
                                   tr("Selection is locked."), disabledCombo,
                                   section),
                    0, 1);
  auto *emptyCombo = new ui::Combobox(section);
  emptyCombo->setPlaceholderText(tr("Open an empty list..."));
  emptyCombo->setError(true);
  combos->addWidget(controlExample(tr("Invalid / empty combobox"),
                                   tr("A choice is required. This example "
                                      "intentionally has no results."),
                                   emptyCombo, section),
                    1, 0, 1, 2);
  layout->addLayout(combos);
  return section;
}

QWidget *GalleryPage::buildTextNumberSection() {
  const auto &s = ui::Theme::instance().spacing();
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(s.space3);
  layout->addWidget(createSectionHeader(
      tr("Textarea & NumberInput"),
      tr("Multiline editing and native integer/decimal controls, including "
         "read-only and invalid states.")));
  auto *textGrid = new QGridLayout();
  textGrid->setSpacing(s.space3);
  textGrid->setColumnStretch(0, 1);
  textGrid->setColumnStretch(1, 1);
  auto *notes = new ui::Textarea(tr("Write a multiline note..."), section);
  auto *readOnly = new ui::Textarea(section);
  readOnly->setPlainText(tr("This text can be selected and copied, but not "
                            "edited.\nLonger notes wrap within the control."));
  readOnly->setReadOnly(true);
  auto *invalid =
      new ui::Textarea(tr("Enter at least 20 characters..."), section);
  invalid->setError(true);
  auto *disabled = new ui::Textarea(section);
  disabled->setPlainText(tr("Disabled multiline control"));
  disabled->setEnabled(false);
  for (auto *editor : {notes, readOnly, invalid, disabled}) {
    editor->setMinimumHeight(s.space12 * 2);
    editor->setMaximumHeight(s.space12 * 3);
  }
  textGrid->addWidget(controlExample(tr("Notes"),
                                     tr("Editable, wrapping plain text."),
                                     notes, section),
                      0, 0);
  textGrid->addWidget(controlExample(tr("Read-only notes"),
                                     tr("Read-only is distinct from disabled."),
                                     readOnly, section),
                      0, 1);
  auto *invalidExample =
      controlExample(tr("Required note"), tr("Enter at least 20 characters."),
                     invalid, section);
  auto *validation = galleryLabel(
      tr("Error: the note needs at least 20 characters."), invalidExample);
  invalidExample->layout()->addWidget(validation);
  connect(invalid, &QPlainTextEdit::textChanged, this, [invalid, validation] {
    const bool tooShort = invalid->toPlainText().trimmed().size() < 20;
    invalid->setError(tooShort);
    const QString message =
        tooShort ? tr("Error: the note needs at least 20 characters.")
                 : tr("The example note length is valid.");
    validation->setText(message);
    invalid->setAccessibleDescription(message);
  });
  textGrid->addWidget(invalidExample, 1, 0);
  textGrid->addWidget(controlExample(tr("Disabled notes"),
                                     tr("Editing is unavailable."), disabled,
                                     section),
                      1, 1);
  layout->addLayout(textGrid);

  auto *numbers = new QGridLayout();
  numbers->setSpacing(s.space3);
  numbers->setColumnStretch(0, 1);
  numbers->setColumnStretch(1, 1);
  auto *integer = new ui::NumberInput(section);
  integer->setRange(0, 100);
  integer->setSingleStep(5);
  integer->setSuffix(tr(" %"));
  integer->setValue(65);
  numbers->addWidget(controlExample(tr("Integer value"),
                                    tr("Range 0–100; step 5."), integer,
                                    section),
                     0, 0);
  auto *decimal = new ui::DecimalInput(section);
  decimal->setDecimals(2);
  decimal->setRange(0.25, 5.0);
  decimal->setSingleStep(0.25);
  decimal->setSuffix(tr(" ×"));
  decimal->setValue(1.25);
  numbers->addWidget(
      controlExample(tr("Decimal value"),
                     tr("Locale-aware formatting; two decimal places."),
                     decimal, section),
      0, 1);
  auto *lockedNumber = new ui::NumberInput(section);
  lockedNumber->setRange(0, 1200);
  lockedNumber->setValue(300);
  lockedNumber->setSuffix(tr(" DPI"));
  lockedNumber->setReadOnly(true);
  numbers->addWidget(controlExample(tr("Read-only integer"),
                                    tr("The value remains selectable."),
                                    lockedNumber, section),
                     1, 0);
  auto *disabledNumber = new ui::DecimalInput(section);
  disabledNumber->setValue(2.5);
  disabledNumber->setEnabled(false);
  numbers->addWidget(controlExample(tr("Disabled decimal"),
                                    tr("The value cannot be changed."),
                                    disabledNumber, section),
                     1, 1);
  layout->addLayout(numbers);
  auto *showErrors =
      new ui::Checkbox(tr("Demonstrate invalid numeric fields"), section);
  auto *numberError = galleryLabel(
      tr("Example error: these two numeric fields are marked invalid."),
      section);
  numberError->hide();
  connect(showErrors, &QCheckBox::toggled, this,
          [integer, decimal, numberError](bool error) {
            integer->setError(error);
            decimal->setError(error);
            numberError->setVisible(error);
            integer->setAccessibleDescription(
                error ? tr("Example error: value marked invalid.")
                      : tr("Range 0–100; step 5."));
            decimal->setAccessibleDescription(
                error ? tr("Example error: value marked invalid.")
                      : tr("Locale-aware formatting; two decimal places."));
          });
  layout->addWidget(showErrors);
  layout->addWidget(numberError);
  return section;
}

QWidget *GalleryPage::buildActionsSection() {
  const auto &s = ui::Theme::instance().spacing();
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(s.space3);
  layout->addWidget(createSectionHeader(
      tr("Menus & Popover"), tr("Dropdown/context actions and anchored content "
                                "with native popup dismissal.")));
  auto *result = galleryLabel(
      tr("Choose an example action; no files are changed."), section);
  auto *row = new QHBoxLayout();
  row->setSpacing(s.space2);
  auto *dropdown =
      new ui::Button(tr("Open dropdown"), ui::ButtonVariant::Outline,
                     ui::ButtonSize::Default, section);
  auto *menu = new ui::DropdownMenu(dropdown);
  auto *preview = menu->addAction(tr("Preview example"));
  preview->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+P")));
  auto *checked = menu->addAction(tr("Show details"));
  checked->setCheckable(true);
  checked->setChecked(true);
  menu->addSeparator();
  menu->addAction(tr("Unavailable action"))->setEnabled(false);
  auto *submenu = new ui::Menu(tr("More actions"), menu);
  submenu->addAction(tr("Inspect example"));
  menu->addMenu(submenu);
  dropdown->setMenu(menu);
  auto reportAction = [result](QAction *action) {
    result->setText(action->isCheckable()
                        ? tr("%1: %2").arg(action->text(), action->isChecked()
                                                               ? tr("on")
                                                               : tr("off"))
                        : tr("Last action: %1").arg(action->text()));
  };
  connect(menu, &QMenu::triggered, this, reportAction);
  row->addWidget(dropdown);

  auto *contextTarget =
      new ui::Button(tr("Context menu"), ui::ButtonVariant::Secondary,
                     ui::ButtonSize::Default, section);
  contextTarget->setToolTip(
      tr("Right-click, or focus this button and press Shift+F10."));
  contextTarget->setAccessibleDescription(contextTarget->toolTip());
  contextTarget->setContextMenuPolicy(Qt::CustomContextMenu);
  auto *contextMenu = new ui::ContextMenu(contextTarget);
  contextMenu->addAction(tr("Inspect context"));
  contextMenu->addSeparator();
  contextMenu->addAction(tr("Disabled context action"))->setEnabled(false);
  connect(contextMenu, &QMenu::triggered, this, reportAction);
  connect(contextTarget, &QWidget::customContextMenuRequested, this,
          [contextTarget, contextMenu](const QPoint &position) {
            const QPoint anchor = position == QPoint(-1, -1)
                                      ? contextTarget->rect().center()
                                      : position;
            contextMenu->popup(contextTarget->mapToGlobal(anchor));
          });
  row->addWidget(contextTarget);

  auto *popoverButton =
      new ui::Button(tr("Open popover"), ui::ButtonVariant::Outline,
                     ui::ButtonSize::Default, section);
  auto *popover = new ui::Popover(popoverButton);
  popover->setAccessibleName(tr("Example popover"));
  popover->contentLayout()->addWidget(
      galleryLabel(tr("Edit an example note"), popover, "bodyText"));
  popover->contentLayout()->addWidget(galleryLabel(
      tr("Escape, the close button, or clicking outside dismisses this popup."),
      popover));
  auto *note = new ui::Input(tr("Example note..."), popover);
  note->setAccessibleName(tr("Popover note"));
  popover->contentLayout()->addWidget(note);
  auto *close = new ui::Button(tr("Close"), ui::ButtonVariant::Secondary,
                               ui::ButtonSize::Small, popover);
  connect(close, &QPushButton::clicked, popover, &ui::Popover::closePopover);
  popover->contentLayout()->addWidget(close);
  connect(popoverButton, &QPushButton::clicked, this, [popover, popoverButton] {
    if (popover->isVisible()) {
      popover->closePopover();
    } else {
      popover->openFor(popoverButton);
    }
  });
  row->addWidget(popoverButton);
  row->addStretch();
  layout->addLayout(row);
  layout->addWidget(galleryLabel(tr("For the context example, right-click or "
                                    "press Shift+F10 on its button."),
                                 section));
  layout->addWidget(result);
  return section;
}

QWidget *GalleryPage::buildStatusSection() {
  const auto &s = ui::Theme::instance().spacing();
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(s.space3);
  layout->addWidget(createSectionHeader(
      tr("Alerts & EmptyState"), tr("Persistent status messages and empty "
                                    "content with an optional action.")));
  layout->addWidget(new ui::Alert(
      tr("Information"),
      tr("All examples are local UI demonstrations. No conversion runs here."),
      ui::AlertVariant::Info, section));
  layout->addWidget(new ui::Alert(tr("Success"),
                                  tr("An example operation completed. This is "
                                     "a sample message, not a test result."),
                                  ui::AlertVariant::Success, section));
  layout->addWidget(
      new ui::Alert(tr("Warning"),
                    tr("Review this example setting before continuing. The "
                       "message remains visible and can be selected."),
                    ui::AlertVariant::Warning, section));
  layout->addWidget(
      new ui::Alert(tr("Error"),
                    tr("The simulated action could not finish. A textual "
                       "severity cue accompanies the color."),
                    ui::AlertVariant::Destructive, section));
  auto *states = new QGridLayout();
  states->setSpacing(s.space3);
  states->setColumnStretch(0, 1);
  states->setColumnStretch(1, 1);
  auto *actionState =
      new ui::EmptyState(tr("No example items"),
                         tr("Use the optional action below to change this "
                            "demonstration's content."),
                         section);
  actionState->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
  actionState->setActionText(tr("Add example item"));
  connect(actionState, &ui::EmptyState::actionTriggered, this,
          [actionState, hasItem = false]() mutable {
            hasItem = !hasItem;
            actionState->setTitle(hasItem ? tr("Example item added")
                                          : tr("No example items"));
            actionState->setDescription(
                hasItem ? tr("Only this demonstration changed. Reset to show "
                             "the empty state again.")
                        : tr("Use the optional action below to change this "
                             "demonstration's content."));
            actionState->setActionText(hasItem ? tr("Reset example")
                                               : tr("Add example item"));
          });
  states->addWidget(actionState, 0, 0);
  auto *passiveState =
      new ui::EmptyState(tr("No matching results"),
                         tr("An empty state can omit the action entirely. Try "
                            "another filter in your application."),
                         section);
  passiveState->setIcon(
      style()->standardIcon(QStyle::SP_FileDialogContentsView));
  states->addWidget(passiveState, 0, 1);
  layout->addLayout(states);
  return section;
}

QWidget *GalleryPage::buildLayoutSection() {
  const auto &s = ui::Theme::instance().spacing();
  auto *section = new QWidget(m_containerWidget);
  auto *layout = new QVBoxLayout(section);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(s.space3);
  layout->addWidget(createSectionHeader(
      tr("ScrollArea & Splitter"),
      tr("Scroll each pane independently. Drag the divider, or focus it and "
         "use arrow keys to resize.")));
  auto *vertical = new ui::Checkbox(tr("Stack panels vertically"), section);
  layout->addWidget(vertical);
  auto *splitter = new ui::Splitter(Qt::Horizontal, section);
  splitter->setAccessibleName(tr("Resizable example panels"));
  splitter->setMinimumHeight(s.space12 * 5);
  splitter->setMaximumHeight(s.space12 * 7);
  for (int pane = 0; pane < 2; ++pane) {
    auto *area = new ui::ScrollArea(splitter);
    area->setAccessibleName(pane == 0 ? tr("First scrollable pane")
                                      : tr("Second scrollable pane"));
    area->setMinimumSize(s.space12 * 3, s.space12 * 2);
    auto *content = new QWidget(area);
    auto *items = new QVBoxLayout(content);
    items->setContentsMargins(s.space3, s.space3, s.space3, s.space3);
    items->setSpacing(s.space2);
    for (int index = 1; index <= 14; ++index) {
      auto *item =
          galleryLabel(tr("Pane %1 — example row %2").arg(pane + 1).arg(index),
                       content, "bodyText");
      item->setMinimumHeight(s.space8);
      items->addWidget(item);
    }
    items->addStretch();
    area->setWidget(content);
    splitter->addWidget(area);
  }
  splitter->setSizes({1, 1});
  connect(vertical, &QCheckBox::toggled, this, [splitter](bool stacked) {
    splitter->setOrientation(stacked ? Qt::Vertical : Qt::Horizontal);
    splitter->setSizes({1, 1});
  });
  layout->addWidget(splitter);
  return section;
}

void GalleryPage::onToggleLoadingClicked() {
  if (m_loadingTestBtn) {
    m_loadingTestBtn->setLoading(!m_loadingTestBtn->isLoading());
  }
}

void GalleryPage::onShowToastSuccessClicked() {
  ui::Toast::show(this, "Operation Completed",
                  "All components rendered and verified successfully.",
                  ui::ToastVariant::Success);
}

void GalleryPage::onShowToastDestructiveClicked() {
  ui::Toast::show(this, "Operation Failed",
                  "Could not complete the simulated action.",
                  ui::ToastVariant::Destructive);
}

void GalleryPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();
  QString bgHex = ui::StyleHelper::toHexString(c.background);
  QString fgHex = ui::StyleHelper::toHexString(c.foreground);
  QString mutedFgHex = ui::StyleHelper::toHexString(c.mutedForeground);

  setStyleSheet(QString("QScrollArea#galleryScrollArea { background-color: %1; "
                        "border: none; }"
                        "QWidget#galleryContainer { background-color: %1; }")
                    .arg(bgHex));

  const auto labels = findChildren<QLabel *>();
  for (QLabel *lbl : labels) {
    QString role = lbl->property("galleryRole").toString();
    if (role == "sectionTitle") {
      lbl->setFont(t.font(t.sizeLg, QFont::Bold));
      lbl->setStyleSheet(
          QString("color: %1; background: transparent;").arg(fgHex));
    } else if (role == "bodyText") {
      lbl->setFont(t.font(t.sizeBase, QFont::Normal));
      lbl->setStyleSheet(
          QString("color: %1; background: transparent;").arg(fgHex));
    } else if (role == "mutedText") {
      lbl->setFont(t.font(t.sizeSm, QFont::Normal));
      lbl->setStyleSheet(
          QString("color: %1; background: transparent;").arg(mutedFgHex));
    }
  }
}
