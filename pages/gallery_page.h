#ifndef GALLERY_PAGE_H
#define GALLERY_PAGE_H

#include <QWidget>

class QScrollArea;
class QVBoxLayout;
class QLabel;

namespace ui {
class Button;
class Spinner;
class Progress;
class Switch;
class Slider;
class Input;
}

class GalleryPage : public QWidget {
  Q_OBJECT

public:
  explicit GalleryPage(QWidget *parent = nullptr);
  ~GalleryPage() override = default;

private slots:
  void applyThemeStyles();
  void onToggleLoadingClicked();
  void onShowToastSuccessClicked();
  void onShowToastDestructiveClicked();

private:
  void setupUi();
  QWidget *createSectionHeader(const QString &title, const QString &description = "");
  QWidget *buildButtonSection();
  QWidget *buildBadgeSection();
  QWidget *buildFormSection();
  QWidget *buildFeedbackSection();
  QWidget *buildContentSection();

  QScrollArea *m_scrollArea{nullptr};
  QWidget *m_containerWidget{nullptr};
  QVBoxLayout *m_contentLayout{nullptr};

  ui::Button *m_loadingTestBtn{nullptr};
  ui::Spinner *m_spinner{nullptr};
  ui::Progress *m_progress{nullptr};
  ui::Slider *m_slider{nullptr};
  ui::Switch *m_switch{nullptr};
};

#endif // GALLERY_PAGE_H
