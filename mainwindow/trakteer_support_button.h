#ifndef TRAKTEER_SUPPORT_BUTTON_H
#define TRAKTEER_SUPPORT_BUTTON_H

#include "ui-kit/components/button.h"
#include <QPixmap>
#include <QPointer>

class QEvent;
class QTimer;
class QVariantAnimation;

class TrakteerSupportButton final : public ui::Button {
  Q_OBJECT

public:
  explicit TrakteerSupportButton(QWidget *calloutHost,
                                 QWidget *parent = nullptr);
  ~TrakteerSupportButton() override = default;

protected:
  bool event(QEvent *e) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void applyThemeStyles();
  void retranslateUi();
  void onClicked();

private:
  void initBadgePixmap();
  void setupAnimations();
  void triggerWiggle();
  void updateBadgeIcon(qreal angleDegrees);
  void scheduleNextCallout();
  void showCallout();
  void hideCallout(bool immediate = false);
  void positionCallout();

  QPointer<QWidget> m_calloutHost;
  QPixmap m_badgePixmap;
  QVariantAnimation *m_wiggleAnim{nullptr};
  QTimer *m_wiggleTimer{nullptr};
  QPointer<QWidget> m_callout;
  QVariantAnimation *m_calloutAnim{nullptr};
  QTimer *m_calloutDelayTimer{nullptr};
  QTimer *m_calloutHideTimer{nullptr};
  int m_calloutShownCount{0};
  bool m_calloutSuppressed{false};
  bool m_calloutHiding{false};
};

#endif // TRAKTEER_SUPPORT_BUTTON_H
