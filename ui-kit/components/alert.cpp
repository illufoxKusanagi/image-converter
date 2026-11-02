#include "alert.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace ui {

Alert::Alert(QWidget *parent)
    : Alert(QString(), QString(), AlertVariant::Info, parent) {}

Alert::Alert(const QString &title, const QString &description,
             AlertVariant variant, QWidget *parent)
    : QFrame(parent), m_variant(variant) {
  setFrameShape(QFrame::NoFrame);
  setAttribute(Qt::WA_StyledBackground, true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  m_layout = new QVBoxLayout(this);
  m_variantLabel = new QLabel(this);
  m_titleLabel = new QLabel(this);
  m_descriptionLabel = new QLabel(this);

  QLabel *labels[] = {m_variantLabel, m_titleLabel, m_descriptionLabel};
  for (QLabel *label : labels) {
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                   Qt::TextSelectableByKeyboard);
    label->setFocusPolicy(Qt::StrongFocus);
    label->setAlignment(Qt::AlignLeading | Qt::AlignTop);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_layout->addWidget(label);
  }

  setTitle(title);
  setDescription(description);
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &Alert::applyThemeStyles);
  applyThemeStyles();
}

void Alert::setTitle(const QString &title) {
  m_titleLabel->setText(title);
  m_titleLabel->setAccessibleName(title);
  m_titleLabel->setVisible(!title.isEmpty());
  setAccessibleName(title.isEmpty() ? m_variantLabel->text() : title);
}

QString Alert::title() const { return m_titleLabel->text(); }

void Alert::setDescription(const QString &description) {
  m_descriptionLabel->setText(description);
  m_descriptionLabel->setAccessibleName(description);
  m_descriptionLabel->setVisible(!description.isEmpty());
  setAccessibleDescription(description);
}

QString Alert::description() const { return m_descriptionLabel->text(); }

void Alert::setVariant(AlertVariant variant) {
  if (m_variant != variant) {
    m_variant = variant;
    applyThemeStyles();
  }
}

AlertVariant Alert::variant() const { return m_variant; }

void Alert::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &s = theme.spacing();
  const auto &r = theme.radius();
  const auto &t = theme.typography();

  QColor indicator = c.info;
  QColor foreground = c.infoForeground;
  QColor surface = c.infoSurface;
  QString cue = tr("Information");
  switch (m_variant) {
  case AlertVariant::Info:
    break;
  case AlertVariant::Success:
    indicator = c.success;
    foreground = c.successForeground;
    surface = c.successSurface;
    cue = tr("Success");
    break;
  case AlertVariant::Warning:
    indicator = c.warning;
    foreground = c.warningForeground;
    surface = c.warningSurface;
    cue = tr("Warning");
    break;
  case AlertVariant::Destructive:
    indicator = c.destructive;
    surface = StyleHelper::tint(c.background, c.destructive,
                                theme.isDark() ? 0.14 : 0.08);
    foreground = StyleHelper::tint(c.destructive, c.foreground, 0.55);
    cue = tr("Error");
    break;
  }

  m_variantLabel->setText(cue);
  m_variantLabel->setAccessibleName(cue);
  setAccessibleName(title().isEmpty() ? cue : title());

  m_layout->setContentsMargins(s.space4, s.space4, s.space4, s.space4);
  m_layout->setSpacing(s.space1);
  m_variantLabel->setFont(t.font(t.sizeSm, QFont::DemiBold));
  m_titleLabel->setFont(t.font(t.sizeBase, QFont::DemiBold));
  m_descriptionLabel->setFont(t.font(t.sizeSm));

  const QString labelStyle = QString("QLabel {"
                                     "  color: %1;"
                                     "  background-color: transparent;"
                                     "  border: none;"
                                     "  margin: %2px;"
                                     "  padding: %2px;"
                                     "}")
                                 .arg(StyleHelper::toHexString(foreground))
                                 .arg(s.space0);
  m_variantLabel->setStyleSheet(labelStyle);
  m_titleLabel->setStyleSheet(labelStyle);
  m_descriptionLabel->setStyleSheet(labelStyle);

  setStyleSheet(QString("ui--Alert {"
                        "  background-color: %1;"
                        "  border: 1px solid %2;"
                        "  border-radius: %3px;"
                        "}")
                    .arg(StyleHelper::toHexString(surface))
                    .arg(StyleHelper::toHexString(indicator))
                    .arg(r.lg));
}

} // namespace ui
