#ifndef UI_POPOVER_H
#define UI_POPOVER_H

#include <QFrame>
#include <QMetaObject>
#include <QPointer>

class QEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QShowEvent;
class QVBoxLayout;

namespace ui {

class Popover : public QFrame {
  Q_OBJECT

public:
  explicit Popover(QWidget *parent = nullptr);
  ~Popover() override;

  QVBoxLayout *contentLayout() const;
  void openFor(QWidget *anchor);
  void closePopover();

signals:
  void opened();
  void closed();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  void changeEvent(QEvent *event) override;

private:
  void applyThemeStyles();
  void positionForAnchor();
  void focusInitialContent();
  void detachAnchor();

  QVBoxLayout *m_layout;
  QPointer<QWidget> m_anchor;
  QPointer<QWidget> m_anchorWindow;
  QPointer<QWidget> m_previousFocus;
  QMetaObject::Connection m_anchorDestroyed;
  quint64 m_openGeneration{0};
  bool m_isOpen{false};
  bool m_allowFocusRestore{false};
};

} // namespace ui

#endif // UI_POPOVER_H
