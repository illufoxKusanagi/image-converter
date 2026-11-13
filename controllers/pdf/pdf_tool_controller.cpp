#include "controllers/pdf/pdf_tool_controller.h"

PdfToolController::PdfToolController(QObject *parent)
    : QObject(parent) {}

PdfToolController::~PdfToolController() {
  cancelAndWait();
}

void PdfToolController::cancel() {
  m_isCancelled = true;
}

void PdfToolController::cancelAndWait() {
  cancel();
  waitForRunningJob();
}

bool PdfToolController::isRunning() const {
  return m_watcher.isRunning();
}

bool PdfToolController::isCancelled() const {
  return m_isCancelled.load();
}

void PdfToolController::waitForRunningJob() {
  if (m_watcher.isRunning()) {
    m_watcher.waitForFinished();
  }
}

