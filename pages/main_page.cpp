#include "main_page.h"
#include "ui-kit/components/dialog.h"
#include "ui-kit/components/toast.h"
#include "ui-kit/theme/icon_helper.h"
#include "ui-kit/theme/style_helper.h"
#include "ui-kit/theme/theme.h"
#include <QBuffer>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPageSize>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QtConcurrent/QtConcurrent>

MainPage::MainPage(QWidget *parent)
    : QWidget(parent), mainLayout(new QVBoxLayout(this)),
      m_futureWatcher(new QFutureWatcher<void>(this)),
      m_targetImageExtension(DropFileWidget::ImageExtension::JPG) {
  mainLayout->setContentsMargins(0, 8, 0, 8);
  mainLayout->setSpacing(14);
  mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  setupImageLayout();
  if (m_targetExtensionSelect) {
    onImageTargetExtensionChanged();
  }
  setLayout(mainLayout);

  connect(&ui::Theme::instance(), &ui::Theme::themeChanged, this,
          &MainPage::applyThemeStyles);
  applyThemeStyles();
}

MainPage::~MainPage() {
  m_isCancelled = true;
  if (m_futureWatcher->isRunning()) {
    m_futureWatcher->waitForFinished();
  }
}

void MainPage::setupExtensionButton() {
  QStringList extensionOptions = {"JPG",  "JPEG", "PNG", "WEBP",
                                  "TIFF", "BMP",  "GIF", "PDF"};
  m_targetExtensionSelect = new ui::Select(extensionOptions, this);
  m_targetExtensionField =
      new ui::FormField("Target Format", m_targetExtensionSelect, this);
  connect(m_targetExtensionSelect,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainPage::onImageTargetExtensionChanged);
}

void MainPage::setupImageAttribute() {
  QHBoxLayout *attributeLayout = new QHBoxLayout();
  attributeLayout->setContentsMargins(0, 0, 0, 0);
  attributeLayout->setSpacing(16);
  setupExtensionButton();
  attributeLayout->addWidget(m_qualitySlider, 1, Qt::AlignTop);
  attributeLayout->addWidget(m_targetExtensionField, 1, Qt::AlignTop);
  mainLayout->addLayout(attributeLayout);
}

void MainPage::setupImageLayout() {
  m_qualitySlider = new SliderWidget(this, "Image Quality");
  m_qualitySlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_dragWidget = new DropFileWidget(this, "Image", m_qualitySlider,
                                    &m_targetImageExtension);

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignCenter);
  m_progressLabel->setVisible(false);

  m_progressBar = new ui::Progress(0, this);
  m_progressBar->setVisible(false);

  m_processButton = new ui::Button("Convert Files", ui::ButtonVariant::Primary,
                                   ui::ButtonSize::Large, this);
  m_processButton->setMinimumWidth(320);
  connect(m_processButton, &QPushButton::clicked, this,
          &MainPage::onProcessButtonClicked);

  m_cancelButton =
      new ui::Button("Cancel Conversion", ui::ButtonVariant::Destructive,
                     ui::ButtonSize::Large, this);
  m_cancelButton->setMinimumWidth(320);
  m_cancelButton->setVisible(false);
  connect(m_cancelButton, &QPushButton::clicked, this,
          &MainPage::onCancelButtonClicked);

  mainLayout->addWidget(m_dragWidget);
  setupImageAttribute();
  mainLayout->addWidget(m_progressLabel);
  mainLayout->addWidget(m_progressBar);
  mainLayout->addWidget(m_processButton, 0, Qt::AlignCenter);
  mainLayout->addWidget(m_cancelButton, 0, Qt::AlignCenter);
}

void MainPage::applyThemeStyles() {
  const auto &c = ui::Theme::instance().colors();
  const auto &t = ui::Theme::instance().typography();

  if (m_progressLabel) {
    m_progressLabel->setFont(t.font(t.sizeXs, QFont::DemiBold));
    m_progressLabel->setStyleSheet(
        QString("color: %1; background: transparent;")
            .arg(ui::StyleHelper::toHexString(c.mutedForeground)));
  }

  if (m_processButton) {
    m_processButton->setIcon(
        ui::Icon::get(":/icons/icons/lucide-refresh-cw.svg",
                      c.primaryForeground, QSize(16, 16)));
  }
  if (m_cancelButton) {
    m_cancelButton->setIcon(ui::Icon::get(
        ":/icons/icons/lucide-x.svg", c.destructiveForeground, QSize(16, 16)));
  }
}

void MainPage::setProcessingState(bool isProcessing) {
  m_isProcessing = isProcessing;
  m_processButton->setVisible(!isProcessing);
  m_cancelButton->setVisible(isProcessing);
  m_cancelButton->setEnabled(isProcessing);
  if (m_progressLabel) {
    m_progressLabel->setVisible(isProcessing);
  }
  if (m_progressBar) {
    m_progressBar->setIndeterminate(false);
    m_progressBar->setValue(0, false);
    m_progressBar->setVisible(isProcessing);
  }
  if (isProcessing) {
    m_dragWidget->beginProcessing();
    m_qualitySlider->setEnabled(false);
  } else {
    m_dragWidget->finishProcessing(m_isCancelled);
    onImageTargetExtensionChanged();
  }
}

void MainPage::onCancelButtonClicked() {
  if (m_isProcessing) {
    m_isCancelled = true;
    m_cancelButton->setText("Cancelling...");
    m_cancelButton->setEnabled(false);
  }
}

void MainPage::onProcessButtonClicked() {
  if (m_isProcessing) {
    return;
  }

  QStringList sourcePaths = m_dragWidget->getFilePaths();
  if (sourcePaths.isEmpty()) {
    ui::Toast::show(this, "No Files Selected",
                    "Please select at least one file.",
                    ui::ToastVariant::Warning);
    return;
  }

  DropFileWidget::ImageExtension targetExt = m_targetImageExtension;
  QString targetFormatString = m_dragWidget->imageExtensionToString(targetExt);
  int quality = m_qualitySlider->getValue();
  using FileStatus = DropFileWidget::FileStatus;
  const auto updateFileStatus = [this](const QString &path, FileStatus status,
                                       const QString &detail) {
    QMetaObject::invokeMethod(
        this,
        [this, path, status, detail]() {
          m_dragWidget->setFileStatus(path, status, detail);
        },
        Qt::QueuedConnection);
  };
  const auto updateProgress = [this](int completed, int total,
                                     const QString &label) {
    QMetaObject::invokeMethod(
        this,
        [this, completed, total, label]() {
          m_progressBar->setRange(0, total);
          m_progressBar->setValue(completed);
          if (!label.isEmpty()) {
            m_progressLabel->setText(label);
          }
        },
        Qt::QueuedConnection);
  };

  // SCENARIO 1: Images -> PDF (Single or Merged Multi-Page)
  if (targetExt == DropFileWidget::ImageExtension::PDF) {
    QString suggestedName;
    if (sourcePaths.size() == 1) {
      suggestedName =
          QFileInfo(sourcePaths.first())
              .dir()
              .filePath(QFileInfo(sourcePaths.first()).baseName() + ".pdf");
    } else {
      suggestedName =
          QFileInfo(sourcePaths.first()).dir().filePath("merged_images.pdf");
    }

    QString outputPath = QFileDialog::getSaveFileName(
        this, sourcePaths.size() == 1 ? "Save PDF" : "Save Merged PDF",
        suggestedName, "PDF Files (*.pdf)");

    if (outputPath.isEmpty()) {
      return;
    }

    m_isCancelled = false;
    setProcessingState(true);
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(totalFiles == 1);
    m_progressLabel->setText(QString("Adding %1 to PDF (1 of %2)...")
                                 .arg(QFileInfo(sourcePaths.first()).fileName())
                                 .arg(totalFiles));

    QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputPath,
                                              quality, totalFiles,
                                              updateFileStatus,
                                              updateProgress]() {
      QString tempOutputPath = outputPath + ".tmp";
      int clampedQuality = qBound(1, quality, 100);
      int targetDpi = 72 + ((300 - 72) * clampedQuality) / 100;
      qreal scaleFactor = 0.35 + (0.65 * clampedQuality) / 100.0;

      QPdfWriter pdfWriter(tempOutputPath);
      pdfWriter.setResolution(targetDpi);
      pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

      qint64 totalOrigBytes = 0;
      int successCount = 0;
      int skippedCount = 0;
      int startedFiles = 0;
      QStringList includedPaths;
      QString failedPath;
      bool ok = true;

      QPainter painter;
      for (int i = 0; i < totalFiles; ++i) {
        if (m_isCancelled) {
          ok = false;
          break;
        }

        const QString &path = sourcePaths.at(i);
        startedFiles++;
        updateFileStatus(path, FileStatus::Processing, "Adding image to PDF");
        updateProgress(i, totalFiles,
                       QString("Adding %1 to PDF (%2 of %3)...")
                           .arg(QFileInfo(path).fileName())
                           .arg(i + 1)
                           .arg(totalFiles));
        QImage origImage(path);
        if (origImage.isNull()) {
          skippedCount++;
          updateFileStatus(path, FileStatus::Failed, "Could not load image");
          updateProgress(i + 1, totalFiles, QString());
          continue;
        }

        // Preserve consistent physical page dimensions in points (at 96 base
        // DPI)
        QSizeF ptSize(origImage.width() * 72.0 / 96.0,
                      origImage.height() * 72.0 / 96.0);
        pdfWriter.setPageSize(QPageSize(ptSize, QPageSize::Point));

        // Downscale & compress image according to quality slider
        QImage processedImg = origImage.convertToFormat(QImage::Format_RGB888);
        if (clampedQuality < 95) {
          QSize scaledSize(qMax(1, qRound(origImage.width() * scaleFactor)),
                           qMax(1, qRound(origImage.height() * scaleFactor)));
          processedImg = processedImg.scaled(scaledSize, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
          QByteArray jpegBytes;
          QBuffer buffer(&jpegBytes);
          if (buffer.open(QIODevice::WriteOnly)) {
            processedImg.save(&buffer, "JPG", clampedQuality);
            buffer.close();
            QImage reloaded;
            if (reloaded.loadFromData(jpegBytes, "JPG")) {
              processedImg = reloaded;
            }
          }
        }

        if (successCount == 0) {
          if (!painter.begin(&pdfWriter)) {
            ok = false;
            failedPath = path;
            break;
          }
          painter.setRenderHint(QPainter::SmoothPixmapTransform);
          painter.setRenderHint(QPainter::Antialiasing);
        } else {
          if (!pdfWriter.newPage()) {
            ok = false;
            failedPath = path;
            break;
          }
        }

        QRect pageRect = pdfWriter.pageLayout().paintRectPixels(targetDpi);
        painter.drawImage(pageRect, processedImg);

        totalOrigBytes += QFileInfo(path).size();
        successCount++;
        includedPaths.append(path);
        updateFileStatus(path, FileStatus::Waiting, "Added; awaiting PDF save");
        updateProgress(i + 1, totalFiles, QString());
      }

      if (ok && !m_isCancelled && successCount > 0) {
        updateProgress(startedFiles, totalFiles,
                       QString("Saving %1 (%2 image(s))...")
                           .arg(QFileInfo(outputPath).fileName())
                           .arg(successCount));
      }
      if (painter.isActive() && !painter.end()) {
        ok = false;
      }

      const bool cancelled = m_isCancelled;
      bool saved = false;
      if (!ok || cancelled || successCount == 0) {
        QFile::remove(tempOutputPath);
      } else {
        const bool hadExisting = QFile::exists(outputPath);
        const QString backupPath = outputPath + ".bak";
        if (hadExisting) {
          QFile::remove(backupPath);
          if (!QFile::rename(outputPath, backupPath)) {
            QFile::remove(tempOutputPath);
            ok = false;
          }
        }
        if (ok && !QFile::rename(tempOutputPath, outputPath)) {
          QFile::remove(tempOutputPath);
          if (hadExisting) {
            QFile::rename(backupPath, outputPath);
          }
          ok = false;
        }
        if (ok && hadExisting) {
          QFile::remove(backupPath);
        }
        saved = ok;
      }

      QMetaObject::invokeMethod(
          this,
          [this, sourcePaths, outputPath, saved, cancelled, successCount,
           skippedCount, totalOrigBytes, includedPaths, failedPath,
           startedFiles]() {
            for (const QString &path : includedPaths) {
              m_dragWidget->setFileStatus(
                  path,
                  saved ? FileStatus::Succeeded
                        : (cancelled ? FileStatus::Cancelled
                                     : FileStatus::Failed),
                  saved ? QString("Saved in %1")
                              .arg(QFileInfo(outputPath).fileName())
                        : (cancelled ? "PDF creation cancelled; no output saved"
                                     : "Failed to save PDF"));
            }
            if (!failedPath.isEmpty()) {
              m_dragWidget->setFileStatus(failedPath, FileStatus::Failed,
                                          "Failed to add image to PDF");
            }
            for (int i = startedFiles; i < sourcePaths.size(); ++i) {
              m_dragWidget->setFileStatus(
                  sourcePaths.at(i),
                  cancelled ? FileStatus::Cancelled : FileStatus::Failed,
                  cancelled ? "Not processed (cancelled)"
                            : "Not processed: PDF creation failed");
            }
            setProcessingState(false);
            m_cancelButton->setText("Cancel Conversion");

            if (saved) {
              qint64 outSize = QFileInfo(outputPath).size();
              QString sizeStats =
                  QString("\n\nTotal Original: %1\nPDF Size: %2")
                      .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                      .arg(DropFileWidget::formatFileSize(outSize));
              QString msg =
                  (successCount == 1)
                      ? QString("Image converted to PDF successfully!%1")
                            .arg(sizeStats)
                      : QString("Successfully merged %1 images into PDF!%2")
                            .arg(successCount)
                            .arg(sizeStats);
              if (skippedCount > 0) {
                msg += QString("\n(%1 invalid image(s) skipped)")
                           .arg(skippedCount);
              }
              ui::Dialog::info(this, "Success", msg);
            } else if (cancelled) {
              ui::Toast::show(this, "Cancelled", "PDF creation was cancelled.",
                              ui::ToastVariant::Default);
            } else {
              ui::Toast::show(this, "Error", "Failed to create PDF!",
                              ui::ToastVariant::Destructive);
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 2: Input contains PDF -> Extract PDF Pages into Images
  bool hasPdfInput = false;
  for (const QString &p : sourcePaths) {
    if (QFileInfo(p).suffix().toLower() == "pdf") {
      hasPdfInput = true;
      break;
    }
  }

  if (hasPdfInput) {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory for Extracted Images", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    m_isCancelled = false;
    setProcessingState(true);
    m_progressBar->setRange(0, sourcePaths.size());
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(sourcePaths.size() == 1);
    m_progressLabel->setText(QString("Processing %1 (file 1 of %2)...")
                                 .arg(QFileInfo(sourcePaths.first()).fileName())
                                 .arg(sourcePaths.size()));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                              quality, targetExt, dragWidget,
                                              updateFileStatus,
                                              updateProgress]() {
      int totalExtracted = 0;
      int failedCount = 0;
      int completedFiles = 0;
      int startedFiles = 0;
      const int totalFiles = sourcePaths.size();
      qint64 totalOutputBytes = 0;
      QSet<QString> usedBaseNames;
      QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

      for (int i = 0; i < sourcePaths.size(); ++i) {
        if (m_isCancelled)
          break;

        const QString &sourcePath = sourcePaths.at(i);
        startedFiles++;
        updateFileStatus(sourcePath, FileStatus::Processing, "Loading source");
        updateProgress(completedFiles, totalFiles,
                       QString("Processing %1 (file %2 of %3)...")
                           .arg(QFileInfo(sourcePath).fileName())
                           .arg(i + 1)
                           .arg(sourcePaths.size()));
        if (QFileInfo(sourcePath).suffix().toLower() == "pdf") {
          QPdfDocument doc;
          if (doc.load(sourcePath) != QPdfDocument::Error::None) {
            failedCount++;
            completedFiles++;
            updateFileStatus(sourcePath, FileStatus::Failed,
                             "Could not load PDF");
            if (totalFiles > 1) {
              updateProgress(completedFiles, totalFiles, QString());
            }
            continue;
          }

          int pageCount = doc.pageCount();
          if (pageCount <= 0) {
            failedCount++;
            completedFiles++;
            updateFileStatus(sourcePath, FileStatus::Failed,
                             "PDF contains no pages");
            if (totalFiles > 1) {
              updateProgress(completedFiles, totalFiles, QString());
            }
            continue;
          }
          if (totalFiles == 1) {
            QMetaObject::invokeMethod(
                this,
                [this, pageCount]() {
                  m_progressBar->setIndeterminate(false);
                  m_progressBar->setRange(0, pageCount);
                  m_progressBar->setValue(0, false);
                },
                Qt::QueuedConnection);
          }
          const auto updatePageProgress =
              [totalFiles, pageCount, &completedFiles,
               updateProgress](int completedPages, const QString &label) {
                updateProgress(totalFiles == 1 ? completedPages
                                               : completedFiles,
                               totalFiles == 1 ? pageCount : totalFiles, label);
              };
          int completedPages = 0;
          int extractedPages = 0;
          int failedPages = 0;
          QString baseName = QFileInfo(sourcePath).baseName();

          for (int p = 0; p < pageCount; ++p) {
            if (m_isCancelled)
              break;

            updateFileStatus(
                sourcePath, FileStatus::Processing,
                QString("Page %1 of %2").arg(p + 1).arg(pageCount));
            updatePageProgress(
                completedPages,
                QString("Extracting %1 (file %2 of %3, page %4 of %5)...")
                    .arg(QFileInfo(sourcePath).fileName())
                    .arg(i + 1)
                    .arg(sourcePaths.size())
                    .arg(p + 1)
                    .arg(pageCount));
            QSizeF ptSize = doc.pagePointSize(p);
            int extractDpi =
                (quality <= 0)
                    ? 72
                    : (72 + static_cast<int>((quality / 100.0) * (600 - 72)));
            qreal scale = extractDpi / 72.0;
            QSize renderSize(
                qMax(1, static_cast<int>(ptSize.width() * scale)),
                qMax(1, static_cast<int>(ptSize.height() * scale)));
            QImage pageImg = doc.render(p, renderSize);

            if (pageImg.isNull()) {
              failedCount++;
              failedPages++;
              completedPages++;
              updatePageProgress(completedPages, QString());
              continue;
            }

            if (pageImg.hasAlphaChannel()) {
              QImage opaquePage(pageImg.size(), QImage::Format_RGB32);
              if (opaquePage.isNull()) {
                failedCount++;
                failedPages++;
                completedPages++;
                updatePageProgress(completedPages, QString());
                continue;
              }
              // PDF viewers supply white paper behind transparent page areas.
              opaquePage.fill(Qt::white);
              opaquePage.setDotsPerMeterX(pageImg.dotsPerMeterX());
              opaquePage.setDotsPerMeterY(pageImg.dotsPerMeterY());
              {
                QPainter painter(&opaquePage);
                painter.drawImage(0, 0, pageImg);
              }
              pageImg = opaquePage;
            }

            QString uniqueName = QString("%1_page%2").arg(baseName).arg(p + 1);
            int suffix = 1;
            while (usedBaseNames.contains(uniqueName.toLower()) ||
                   QFile::exists(
                       QDir(outputDir).filePath(uniqueName + "." + ext))) {
              uniqueName = QString("%1_page%2_%3")
                               .arg(baseName)
                               .arg(p + 1)
                               .arg(suffix++);
            }
            usedBaseNames.insert(uniqueName.toLower());

            if (m_isCancelled)
              break;
            QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
            if (dragWidget->saveImage(&pageImg, baseOutputPath, quality,
                                      &targetExt)) {
              totalExtracted++;
              extractedPages++;
              totalOutputBytes += QFileInfo(baseOutputPath + "." + ext).size();
            } else {
              failedCount++;
              failedPages++;
            }

            completedPages++;
            updatePageProgress(completedPages, QString());
          }

          if (extractedPages + failedPages < pageCount) {
            updateFileStatus(
                sourcePath, FileStatus::Cancelled,
                QString("Cancelled: %1 of %2 pages saved; %3 failed")
                    .arg(extractedPages)
                    .arg(pageCount)
                    .arg(failedPages));
            break;
          } else if (failedPages > 0) {
            updateFileStatus(sourcePath, FileStatus::Failed,
                             QString("%1 of %2 pages saved; %3 failed")
                                 .arg(extractedPages)
                                 .arg(pageCount)
                                 .arg(failedPages));
          } else {
            updateFileStatus(sourcePath, FileStatus::Succeeded,
                             QString("Saved %1 page(s)").arg(extractedPages));
          }
        } else {
          QImage image(sourcePath);
          FileStatus status = FileStatus::Failed;
          QString detail = "Could not load image";
          if (image.isNull()) {
            failedCount++;
          } else {
            QString baseName = QFileInfo(sourcePath).baseName();
            QString uniqueName = baseName;
            int suffix = 1;
            while (usedBaseNames.contains(uniqueName.toLower()) ||
                   QFile::exists(
                       QDir(outputDir).filePath(uniqueName + "." + ext))) {
              uniqueName = QString("%1_%2").arg(baseName).arg(suffix++);
            }
            usedBaseNames.insert(uniqueName.toLower());

            QString baseOutputPath = QDir(outputDir).filePath(uniqueName);
            if (m_isCancelled) {
              status = FileStatus::Cancelled;
              detail = "Cancelled before save";
            } else {
              if (dragWidget->saveImage(&image, baseOutputPath, quality,
                                        &targetExt)) {
                totalExtracted++;
                totalOutputBytes +=
                    QFileInfo(baseOutputPath + "." + ext).size();
                status = FileStatus::Succeeded;
                detail = QString("Saved as %1.%2").arg(uniqueName).arg(ext);
              } else {
                failedCount++;
                detail = QString("Failed to save as %1").arg(ext.toUpper());
              }
            }
          }
          updateFileStatus(sourcePath, status, detail);
          if (status == FileStatus::Cancelled) {
            break;
          }
        }
        completedFiles++;
        if (totalFiles > 1) {
          updateProgress(completedFiles, totalFiles, QString());
        }
      }

      const bool cancelled = m_isCancelled && completedFiles < totalFiles;
      QMetaObject::invokeMethod(
          this,
          [this, sourcePaths, startedFiles, cancelled, totalExtracted,
           failedCount, totalOutputBytes, outputDir]() {
            for (int i = startedFiles; i < sourcePaths.size(); ++i) {
              m_dragWidget->setFileStatus(sourcePaths.at(i),
                                          FileStatus::Cancelled,
                                          "Not processed (cancelled)");
            }
            setProcessingState(false);
            m_cancelButton->setText("Cancel Conversion");

            if (cancelled) {
              ui::Toast::show(this, "Cancelled", "Extraction was cancelled.",
                              ui::ToastVariant::Default);
            } else if (totalExtracted > 0) {
              QString msg =
                  QString("Extracted %1 image(s) to:\n%2\n\nTotal Size: %3")
                      .arg(totalExtracted)
                      .arg(outputDir)
                      .arg(DropFileWidget::formatFileSize(totalOutputBytes));
              if (failedCount > 0) {
                msg += QString("\n(%1 failed)").arg(failedCount);
              }
              ui::Dialog::info(this, "Success", msg);
            } else {
              ui::Toast::show(this, "Error",
                              "Failed to extract images from PDF!",
                              ui::ToastVariant::Destructive);
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
    return;
  }

  // SCENARIO 3: Standard Image -> Image Conversion (Single & Batch)
  if (sourcePaths.size() == 1) {
    const QString &sourcePath = sourcePaths.first();
    QString filter = targetFormatString.toUpper() + " (*." +
                     targetFormatString.toLower() + ")";
    QString suggestedName =
        QFileInfo(sourcePath)
            .dir()
            .filePath(QFileInfo(sourcePath).baseName() + "_converted");

    QString outputPathWithExt = QFileDialog::getSaveFileName(
        this, "Save Image As", suggestedName, filter);

    if (outputPathWithExt.isEmpty()) {
      return;
    }

    const QString targetSuffix = targetFormatString.toLower();
    if (QFileInfo(outputPathWithExt).suffix().toLower() != targetSuffix) {
      outputPathWithExt += "." + targetSuffix;
    }

    QFileInfo outFi(outputPathWithExt);
    QString outputPathWithoutExt =
        outFi.dir().filePath(outFi.completeBaseName());

    m_isCancelled = false;
    setProcessingState(true);
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(true);
    m_progressLabel->setText(QString("Converting %1 (1 of 1)...")
                                 .arg(QFileInfo(sourcePath).fileName()));
    m_dragWidget->setFileStatus(sourcePath, FileStatus::Processing,
                                "Converting image");

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run([this, sourcePath,
                                              outputPathWithoutExt,
                                              outputPathWithExt, quality,
                                              targetExt, targetFormatString,
                                              dragWidget]() {
      QImage image(sourcePath);
      FileStatus status = FileStatus::Failed;
      QString detail;
      if (image.isNull()) {
        detail = "Could not load image";
      } else if (m_isCancelled) {
        status = FileStatus::Cancelled;
        detail = "Cancelled before save";
      } else if (dragWidget->saveImage(&image, outputPathWithoutExt, quality,
                                       &targetExt)) {
        status = FileStatus::Succeeded;
        detail =
            QString("Saved as %1").arg(QFileInfo(outputPathWithExt).fileName());
      } else {
        detail = QString("Failed to save image as %1").arg(targetFormatString);
      }

      QMetaObject::invokeMethod(
          this,
          [this, sourcePath, status, detail, outputPathWithExt]() {
            m_dragWidget->setFileStatus(sourcePath, status, detail);
            m_progressBar->setValue(status == FileStatus::Cancelled ? 0 : 1,
                                    false);
            setProcessingState(false);
            m_cancelButton->setText("Cancel Conversion");

            if (status == FileStatus::Succeeded) {
              qint64 origSize = QFileInfo(sourcePath).size();
              qint64 outSize = QFileInfo(outputPathWithExt).size();
              QString sizeStats;
              if (origSize > 0 && outSize > 0) {
                double diffPct =
                    (1.0 - (static_cast<double>(outSize) / origSize)) * 100.0;
                sizeStats = QString("\n\nOriginal: %1\nConverted: %2 (%3% %4)")
                                .arg(DropFileWidget::formatFileSize(origSize))
                                .arg(DropFileWidget::formatFileSize(outSize))
                                .arg(qAbs(diffPct), 0, 'f', 1)
                                .arg(diffPct >= 0 ? "smaller" : "larger");
              }

              ui::Dialog::info(this, "Success",
                               QString("Image converted successfully to %1%2")
                                   .arg(outputPathWithExt)
                                   .arg(sizeStats));
            } else if (status == FileStatus::Cancelled) {
              ui::Toast::show(this, "Cancelled",
                              "Image conversion was cancelled.",
                              ui::ToastVariant::Default);
            } else {
              ui::Toast::show(this, "Error", detail,
                              ui::ToastVariant::Destructive);
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
  } else {
    QString outputDir = QFileDialog::getExistingDirectory(
        this, "Select Output Directory", QDir::homePath());
    if (outputDir.isEmpty()) {
      return;
    }

    m_isCancelled = false;
    setProcessingState(true);
    int totalFiles = sourcePaths.size();
    m_progressBar->setRange(0, totalFiles);
    m_progressBar->setValue(0, false);
    m_progressBar->setIndeterminate(false);
    m_progressLabel->setText(QString("Converting %1 (1 of %2)...")
                                 .arg(QFileInfo(sourcePaths.first()).fileName())
                                 .arg(totalFiles));

    DropFileWidget *dragWidget = m_dragWidget;
    QFuture<void> future = QtConcurrent::run([this, sourcePaths, outputDir,
                                              quality, targetExt, totalFiles,
                                              dragWidget, updateFileStatus,
                                              updateProgress]() {
      int successCount = 0;
      int failureCount = 0;
      int completedFiles = 0;
      qint64 totalOrigBytes = 0;
      qint64 totalOutputBytes = 0;
      QSet<QString> usedBaseNames;
      QString ext = dragWidget->imageExtensionToString(targetExt).toLower();

      for (int i = 0; i < totalFiles; ++i) {
        if (m_isCancelled) {
          break;
        }

        const QString &sourcePath = sourcePaths.at(i);
        updateFileStatus(sourcePath, FileStatus::Processing,
                         "Converting image");
        updateProgress(completedFiles, totalFiles,
                       QString("Converting %1 (%2 of %3)...")
                           .arg(QFileInfo(sourcePath).fileName())
                           .arg(i + 1)
                           .arg(totalFiles));
        QImage image(sourcePath);
        FileStatus status = FileStatus::Failed;
        QString detail = "Could not load image";
        if (image.isNull()) {
          failureCount++;
        } else {
          QFileInfo fileInfo(sourcePath);
          QString baseName = fileInfo.baseName();
          QString uniqueBaseName = baseName;
          int suffix = 1;
          while (usedBaseNames.contains(uniqueBaseName.toLower()) ||
                 QFile::exists(
                     QDir(outputDir).filePath(uniqueBaseName + "." + ext))) {
            uniqueBaseName = QString("%1_%2").arg(baseName).arg(suffix++);
          }
          usedBaseNames.insert(uniqueBaseName.toLower());

          if (m_isCancelled)
            break;
          QString baseOutputName = QDir(outputDir).filePath(uniqueBaseName);
          if (dragWidget->saveImage(&image, baseOutputName, quality,
                                    &targetExt)) {
            successCount++;
            totalOrigBytes += fileInfo.size();
            QString outputFilePath = baseOutputName + "." + ext;
            totalOutputBytes += QFileInfo(outputFilePath).size();
            status = FileStatus::Succeeded;
            detail = QString("Saved as %1.%2").arg(uniqueBaseName).arg(ext);
          } else {
            failureCount++;
            detail = QString("Failed to save as %1").arg(ext.toUpper());
          }
        }

        completedFiles++;
        updateFileStatus(sourcePath, status, detail);
        updateProgress(completedFiles, totalFiles, QString());
      }

      const bool cancelled = m_isCancelled && completedFiles < totalFiles;
      QMetaObject::invokeMethod(
          this,
          [this, sourcePaths, completedFiles, cancelled, successCount,
           failureCount, totalOrigBytes, totalOutputBytes]() {
            for (int i = completedFiles; i < sourcePaths.size(); ++i) {
              m_dragWidget->setFileStatus(sourcePaths.at(i),
                                          FileStatus::Cancelled,
                                          "Not processed (cancelled)");
            }
            setProcessingState(false);
            m_cancelButton->setText("Cancel Conversion");

            QString title = cancelled ? "Batch Conversion Cancelled"
                                      : "Batch Conversion Complete";
            QString message =
                QString(
                    "%1 image(s) converted successfully.\n%2 image(s) failed.")
                    .arg(successCount)
                    .arg(failureCount);

            if (totalOrigBytes > 0 && totalOutputBytes > 0 &&
                successCount > 0) {
              double diffPct = (1.0 - (static_cast<double>(totalOutputBytes) /
                                       totalOrigBytes)) *
                               100.0;
              message +=
                  QString(
                      "\n\nTotal Original: %1\nTotal Converted: %2 (%3% %4)")
                      .arg(DropFileWidget::formatFileSize(totalOrigBytes))
                      .arg(DropFileWidget::formatFileSize(totalOutputBytes))
                      .arg(qAbs(diffPct), 0, 'f', 1)
                      .arg(diffPct >= 0 ? "smaller" : "larger");
            }

            if (cancelled) {
              message += "\n(Operation was stopped early by user)";
              ui::Toast::show(this, "Cancelled", message,
                              ui::ToastVariant::Default);
            } else {
              ui::Dialog::info(this, title, message);
            }
          },
          Qt::QueuedConnection);
    });
    m_futureWatcher->setFuture(future);
  }
}

void MainPage::onImageTargetExtensionChanged() {
  int value =
      m_targetExtensionSelect ? m_targetExtensionSelect->currentIndex() : 0;
  if (value == 0) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 1) {
    m_targetImageExtension = DropFileWidget::ImageExtension::JPEG;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 2) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PNG;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("PNG (Lossless - Quality N/A)");
  } else if (value == 3) {
    m_targetImageExtension = DropFileWidget::ImageExtension::WEBP;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 4) {
    m_targetImageExtension = DropFileWidget::ImageExtension::TIFF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("Image Quality");
  } else if (value == 5) {
    m_targetImageExtension = DropFileWidget::ImageExtension::BMP;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("BMP (Uncompressed)");
  } else if (value == 6) {
    m_targetImageExtension = DropFileWidget::ImageExtension::GIF;
    m_qualitySlider->setEnabled(false);
    m_qualitySlider->setTitle("GIF (Indexed Color)");
  } else if (value == 7) {
    m_targetImageExtension = DropFileWidget::ImageExtension::PDF;
    m_qualitySlider->setEnabled(true);
    m_qualitySlider->setTitle("PDF Image Quality");
  }
}
