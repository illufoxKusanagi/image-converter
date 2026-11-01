#ifndef DROPFILEWIDGET_H
#define DROPFILEWIDGET_H

#include "ui-kit/components/badge.h"
#include "ui-kit/components/button.h"
#include "ui-kit/components/separator.h"
#include "ui-kit/components/spinner.h"
#include "ui-kit/theme/theme.h"
#include "widgets/slider_widget.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class DropFileWidget : public QGroupBox {
  Q_OBJECT
public:
  enum ImageExtension { JPG, JPEG, PNG, WEBP, TIFF, BMP, GIF, PDF };
  enum class FileStatus {
    Waiting,
    Processing,
    Succeeded,
    Unchanged,
    Failed,
    Cancelled
  };
  Q_ENUM(FileStatus)

  explicit DropFileWidget(QWidget *parent = nullptr, QString typeFile = "File",
                          SliderWidget *sliderWidget = nullptr,
                          ImageExtension *sourceExtension = nullptr);
  QString getFilePath() const;
  QString filePath() const { return getFilePath(); }
  QStringList getFilePaths() const;
  QStringList filePaths() const { return getFilePaths(); }
  // Call the processing API on the GUI thread, using paths from filePaths().
  void beginProcessing();
  void setFileStatus(const QString &path, FileStatus status,
                     const QString &detail = QString());
  void finishProcessing(bool cancelled = false);
  void clearFiles();
  void removeFileAt(int index);
  void moveFileUp(int index);
  void moveFileDown(int index);
  QString imageExtensionToString(const ImageExtension &sourceExtension) const;
  bool saveImage(const QImage *image, const QString &outputPath,
                 const int quality, const ImageExtension *sourceExtension);
  static QString formatFileSize(qint64 bytes);

public slots:
  void convertImage(const QString &sourcePath);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private slots:
  void onBrowseButtonPressed();
  void onSliderValueChanged();
  void onClearAllPressed();
  void applyThemeStyles();

private:
  struct FileState {
    FileStatus status = FileStatus::Waiting;
    QString detail;
  };

  struct FileRow {
    QWidget *card = nullptr;
    QLabel *icon = nullptr;
    ui::Spinner *spinner = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *upButton = nullptr;
    QPushButton *downButton = nullptr;
    QPushButton *removeButton = nullptr;
  };

  QVBoxLayout *mainLayout;
  SliderWidget *m_sliderWidget;
  ImageExtension *m_sourceExtension;
  ui::Button *m_browseButton;
  QLabel *m_icon;
  QLabel *m_emptyLabel;
  QLabel *m_formatLabel;
  QLabel *m_orLabel;
  int m_qualityValue;
  QString m_sourcePath;
  QStringList m_filePaths;
  QHash<QString, FileState> m_fileStates;
  QHash<QString, FileRow> m_fileRows;
  QString m_typeFile;
  bool m_isDragHovered;
  bool m_isProcessing = false;

  QWidget *m_emptyFieldWidget;
  QWidget *m_chosenFileWidget;
  QLabel *m_chosenHeaderLabel;
  ui::Button *m_clearAllButton;
  ui::Button *m_addMoreButton;
  QScrollArea *m_fileScrollArea;
  QWidget *m_fileListContainer;
  QVBoxLayout *m_fileListLayout;

  void updateDropZoneStyle();
  void setupOrSeparatorLayout(QVBoxLayout *layout);
  void setupEmptyFileWidget();
  void setupChosenFileWidget();
  void updateWidgetVisibility();
  void rebuildFileList();
  void updateFileRowStatus(const QString &path);
  void updateProcessingControls();
  QPixmap createColoredIcon(const QString &iconPath, const QColor &color,
                            int width, int height);
};

#endif // DROPFILEWIDGET_H
