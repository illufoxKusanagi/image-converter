#include "empty_state.h"
#include "../theme/style_helper.h"
#include "../theme/theme.h"
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace ui {

EmptyState::EmptyState(QWidget *parent)
    : EmptyState(QString(), QString(), parent) {}

EmptyState::EmptyState(const QString &title, const QString &description,
                       QWidget *parent)
    : QFrame(parent) {
  setFrameShape(QFrame::NoFrame);
  setAttribute(Qt::WA_StyledBackground, true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  m_layout = new QVBoxLayout(this);
  m_layout->setAlignment(Qt::AlignVCenter);
  m_iconLabel = new QLabel(this);
  m_iconLabel->setAlignment(Qt::AlignCenter);
  m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  m_layout->addWidget(m_iconLabel, 0, Qt::AlignHCenter);

  m_titleLabel = new QLabel(this);
  m_descriptionLabel = new QLabel(this);
  QLabel *labels[] = {m_titleLabel, m_descriptionLabel};
  for (QLabel *label : labels) {
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                   Qt::TextSelectableByKeyboard);
    label->setFocusPolicy(Qt::StrongFocus);
    label->setAlignment(Qt::AlignCenter);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_layout->addWidget(label);
  }

  m_actionButton = new Button(this);
  // A long action caption must not impose a minimum width on the whole state.
  m_actionButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_layout->addWidget(m_actionButton);
  connect(m_actionButton, &Button::clicked, this, &EmptyState::actionTriggered);

  setTitle(title);
  setDescription(description);
  setActionText(QString());
  connect(&Theme::instance(), &Theme::themeChanged, this,
          &EmptyState::applyThemeStyles);
  applyThemeStyles();
}

void EmptyState::setTitle(const QString &title) {
  m_titleLabel->setText(title);
  m_titleLabel->setAccessibleName(title);
  m_titleLabel->setVisible(!title.isEmpty());
  setAccessibleName(title);
}

QString EmptyState::title() const { return m_titleLabel->text(); }

void EmptyState::setDescription(const QString &description) {
  m_descriptionLabel->setText(description);
  m_descriptionLabel->setAccessibleName(description);
  m_descriptionLabel->setVisible(!description.isEmpty());
  setAccessibleDescription(description);
}

QString EmptyState::description() const { return m_descriptionLabel->text(); }

void EmptyState::setIcon(const QIcon &icon) {
  m_icon = icon;
  updateIcon();
}

void EmptyState::setActionText(const QString &text) {
  m_actionButton->setText(text);
  m_actionButton->setAccessibleName(text);
  m_actionButton->setVisible(!text.isEmpty());
}

Button *EmptyState::actionButton() const { return m_actionButton; }

void EmptyState::updateIcon() {
  const auto &theme = Theme::instance();
  const int extent = theme.spacing().space10;
  QPixmap pixmap = m_icon.pixmap(QSize(extent, extent));
  if (pixmap.isNull()) {
    m_iconLabel->clear();
    m_iconLabel->hide();
    return;
  }

  {
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), theme.colors().mutedForeground);
  }
  m_iconLabel->setPixmap(pixmap);
  m_iconLabel->show();
}

void EmptyState::applyThemeStyles() {
  const auto &theme = Theme::instance();
  const auto &c = theme.colors();
  const auto &s = theme.spacing();
  const auto &r = theme.radius();
  const auto &t = theme.typography();

  m_layout->setContentsMargins(s.space6, s.space6, s.space6, s.space6);
  m_layout->setSpacing(s.space3);
  m_iconLabel->setFixedSize(s.space10, s.space10);
  m_titleLabel->setFont(t.font(t.sizeLg, QFont::DemiBold));
  m_descriptionLabel->setFont(t.font(t.sizeSm));

  const QString labelStyle = QString("QLabel {"
                                     "  color: %1;"
                                     "  background-color: transparent;"
                                     "  border: none;"
                                     "  margin: %2px;"
                                     "  padding: %2px;"
                                     "}");
  m_titleLabel->setStyleSheet(
      labelStyle.arg(StyleHelper::toHexString(c.cardForeground)).arg(s.space0));
  m_descriptionLabel->setStyleSheet(
      labelStyle.arg(StyleHelper::toHexString(c.mutedForeground))
          .arg(s.space0));
  m_iconLabel->setStyleSheet(
      labelStyle.arg(StyleHelper::toHexString(c.mutedForeground))
          .arg(s.space0));

  setStyleSheet(QString("ui--EmptyState {"
                        "  background-color: %1;"
                        "  border: 1px solid %2;"
                        "  border-radius: %3px;"
                        "}")
                    .arg(StyleHelper::toHexString(c.card))
                    .arg(StyleHelper::toHexString(c.border))
                    .arg(r.lg));
  updateIcon();
}

} // namespace ui
