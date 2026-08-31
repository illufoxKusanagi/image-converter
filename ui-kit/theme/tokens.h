#ifndef UI_TOKENS_H
#define UI_TOKENS_H

#include <QColor>
#include <QFont>
#include <QString>

namespace ui {

enum class ThemeMode {
  Light,
  Dark
};

struct ColorTokens {
  QColor background;
  QColor foreground;

  QColor card;
  QColor cardForeground;

  QColor popover;
  QColor popoverForeground;

  QColor primary;
  QColor primaryForeground;

  QColor secondary;
  QColor secondaryForeground;

  QColor muted;
  QColor mutedForeground;

  QColor accent;
  QColor accentForeground;

  QColor destructive;
  QColor destructiveForeground;

  QColor border;
  QColor input;
  QColor ring;
};

struct RadiusTokens {
  int none = 0;
  int sm = 4;
  int md = 6;
  int lg = 8;
  int xl = 12;
  int full = 9999;
};

struct SpacingTokens {
  int space0 = 0;
  int space1 = 4;
  int space2 = 8;
  int space3 = 12;
  int space4 = 16;
  int space5 = 20;
  int space6 = 24;
  int space8 = 32;
  int space10 = 40;
  int space12 = 48;
};

struct TypographyTokens {
  QString fontFamily = "Segoe UI, Inter, -apple-system, Roboto, sans-serif";
  int sizeXs = 11;
  int sizeSm = 12;
  int sizeBase = 14;
  int sizeLg = 16;
  int sizeXl = 18;
  int size2Xl = 24;
  int size3Xl = 30;

  QFont font(int pointSize, QFont::Weight weight = QFont::Normal) const {
    QFont f(fontFamily, pointSize);
    f.setWeight(weight);
    return f;
  }
};

inline ColorTokens lightColorTokens() {
  ColorTokens t;
  t.background = QColor("#ffffff");
  t.foreground = QColor("#09090b");

  t.card = QColor("#ffffff");
  t.cardForeground = QColor("#09090b");

  t.popover = QColor("#ffffff");
  t.popoverForeground = QColor("#09090b");

  t.primary = QColor("#18181b");
  t.primaryForeground = QColor("#fafafa");

  t.secondary = QColor("#f4f4f5");
  t.secondaryForeground = QColor("#18181b");

  t.muted = QColor("#f4f4f5");
  t.mutedForeground = QColor("#71717a");

  t.accent = QColor("#f4f4f5");
  t.accentForeground = QColor("#18181b");

  t.destructive = QColor("#ef4444");
  t.destructiveForeground = QColor("#fafafa");

  t.border = QColor("#e4e4e7");
  t.input = QColor("#e4e4e7");
  t.ring = QColor("#18181b");
  return t;
}

inline ColorTokens darkColorTokens() {
  ColorTokens t;
  t.background = QColor("#09090b");
  t.foreground = QColor("#fafafa");

  t.card = QColor("#18181b");
  t.cardForeground = QColor("#fafafa");

  t.popover = QColor("#18181b");
  t.popoverForeground = QColor("#fafafa");

  t.primary = QColor("#fafafa");
  t.primaryForeground = QColor("#18181b");

  t.secondary = QColor("#27272a");
  t.secondaryForeground = QColor("#fafafa");

  t.muted = QColor("#27272a");
  t.mutedForeground = QColor("#a1a1aa");

  t.accent = QColor("#27272a");
  t.accentForeground = QColor("#fafafa");

  t.destructive = QColor("#ef4444");
  t.destructiveForeground = QColor("#fafafa");

  t.border = QColor("#27272a");
  t.input = QColor("#27272a");
  t.ring = QColor("#d4d4d8");
  return t;
}

} // namespace ui

#endif // UI_TOKENS_H
