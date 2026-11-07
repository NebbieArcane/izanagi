#include "file_dialogs.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>

namespace eleutheria_ui {
namespace {

void prepare_parent(QWidget* parent) {
  if(parent == nullptr) {
    return;
  }
  parent->raise();
  parent->activateWindow();
  QApplication::processEvents();
}

QString default_start(const QString& start_dir) {
  if(!start_dir.isEmpty()) {
    return start_dir;
  }
  const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
  return home.isEmpty() ? QStringLiteral("/") : home;
}

void configure_browser(QFileDialog* dialog) {
  // Dialogo Qt (non nativo): sempre una maschera navigabile su Win/Mac/Linux.
  dialog->setOption(QFileDialog::DontUseNativeDialog, true);
  dialog->setViewMode(QFileDialog::Detail);
  dialog->setOption(QFileDialog::DontUseCustomDirectoryIcons, false);
  dialog->setModal(true);
}

}  // namespace

QString chooseExistingDirectory(QWidget* parent, const QString& title, const QString& start_dir) {
  prepare_parent(parent);
  QFileDialog dialog(parent, title, default_start(start_dir));
  configure_browser(&dialog);
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setOption(QFileDialog::ShowDirsOnly, true);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  if(dialog.exec() != QDialog::Accepted) {
    return {};
  }
  const auto files = dialog.selectedFiles();
  return files.isEmpty() ? QString{} : files.front();
}

QString chooseOpenFile(QWidget* parent, const QString& title, const QString& filter,
                       const QString& start_dir) {
  prepare_parent(parent);
  QFileDialog dialog(parent, title, default_start(start_dir), filter);
  configure_browser(&dialog);
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  if(dialog.exec() != QDialog::Accepted) {
    return {};
  }
  const auto files = dialog.selectedFiles();
  return files.isEmpty() ? QString{} : files.front();
}

QString chooseSaveFile(QWidget* parent, const QString& title, const QString& filter,
                       const QString& start_dir, const QString& suggested_name) {
  prepare_parent(parent);
  QString start = default_start(start_dir);
  if(!suggested_name.isEmpty()) {
    start += QLatin1Char('/') + suggested_name;
  }
  QFileDialog dialog(parent, title, start, filter);
  configure_browser(&dialog);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setDefaultSuffix(QStringLiteral("ele"));
  if(dialog.exec() != QDialog::Accepted) {
    return {};
  }
  const auto files = dialog.selectedFiles();
  return files.isEmpty() ? QString{} : files.front();
}

}  // namespace eleutheria_ui
