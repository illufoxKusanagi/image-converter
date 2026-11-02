#ifndef UI_COMBOBOX_H
#define UI_COMBOBOX_H

#include <QModelIndex>
#include <QStringList>
#include <QVariant>
#include <QWidget>

class QEvent;
class QHBoxLayout;
class QHideEvent;
class QLabel;
class QLineEdit;
class QListView;
class QSortFilterProxyModel;
class QStandardItemModel;
class QToolButton;

namespace ui {

class Popover;

class Combobox : public QWidget {
  Q_OBJECT

public:
  explicit Combobox(QWidget *parent = nullptr);
  explicit Combobox(const QStringList &items, QWidget *parent = nullptr);
  ~Combobox() override;

  void setItems(const QStringList &items);
  void addItem(const QString &text, const QVariant &data = QVariant());

  int currentIndex() const;
  void setCurrentIndex(int index);
  QString currentText() const;
  QVariant currentData() const;

  void setPlaceholderText(const QString &placeholder);
  bool hasError() const;
  void setError(bool hasError);

signals:
  void currentIndexChanged(int index);
  void currentTextChanged(const QString &text);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void changeEvent(QEvent *event) override;
  void hideEvent(QHideEvent *event) override;

private:
  void applyThemeStyles();
  void showPopup(const QString &query = QString());
  void updateFilter(const QString &query);
  void updateResults();
  void restoreCommittedText();
  void moveHighlight(int direction);
  void commitIndex(const QModelIndex &proxyIndex);
  bool isSelectable(const QModelIndex &index) const;
  void notifySelectionChanged(int oldIndex, const QString &oldText);

  QHBoxLayout *m_layout;
  QLineEdit *m_lineEdit;
  QToolButton *m_button;
  Popover *m_popup;
  QLineEdit *m_searchEdit;
  QListView *m_list;
  QLabel *m_emptyLabel;
  QStandardItemModel *m_model;
  QSortFilterProxyModel *m_proxy;
  int m_currentIndex{-1};
  bool m_hasError{false};
  bool m_openingPopup{false};
};

} // namespace ui

#endif // UI_COMBOBOX_H
