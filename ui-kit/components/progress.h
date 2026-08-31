#ifndef UI_PROGRESS_H
#define UI_PROGRESS_H

#include "../theme/theme.h"
#include <QPaintEvent>
#include <QVariantAnimation>
#include <QWidget>

namespace ui {

enum class ProgressSize {
  Small,   // 4px
  Default, // 8px
  Large    // 12px
};

class Progress : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue)

public:
  explicit Progress(QWidget *parent = nullptr);
  explicit Progress(int value, QWidget *parent = nullptr);
  ~Progress() override = default;

  int value() const;
  void setValue(int value, bool animated = true);

  int minimum() const;
  void setMinimum(int min);

  int maximum() const;
  void setMaximum(int max);

  void setRange(int min, int max);

  bool isIndeterminate() const;
  void setIndeterminate(bool indeterminate);

  ProgressSize size() const;
  void setSize(ProgressSize size);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private slots:
  void applyThemeStyles();

private:
  int m_value{0};
  int m_displayValue{0};
  int m_min{0};
  int m_max{100};
  bool m_isIndeterminate{false};
  ProgressSize m_size{ProgressSize::Default};

  qreal m_indeterminatePos{0.0};
  QVariantAnimation *m_valueAnim{nullptr};
  QVariantAnimation *m_indeterminateAnim{nullptr};

  int trackHeight() const;
};

} // namespace ui

#endif // UI_PROGRESS_H
