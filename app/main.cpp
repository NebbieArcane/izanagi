#include "main_window.hpp"

#include <QApplication>
#include <QIcon>
#include <QTimer>

namespace {

QIcon load_app_icon() {
  QIcon icon;
  const int sizes[] = {16, 32, 48, 64, 128, 256, 512, 1024};
  for(int size : sizes) {
    icon.addFile(QStringLiteral(":/icons/izanagi-%1.png").arg(size), QSize(size, size));
  }
  return icon;
}

}  // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("Izanagi");
  QApplication::setOrganizationName("NebbieArcane");
  QApplication::setOrganizationDomain("nebbiearcane.org");
  QApplication::setApplicationVersion("0.1.0");
  const QIcon app_icon = load_app_icon();
  QApplication::setWindowIcon(app_icon);

  MainWindow window;
  window.setWindowIcon(app_icon);
  window.show();
  window.raise();
  window.activateWindow();

  if(argc > 1) {
    const QString path = QString::fromLocal8Bit(argv[1]);
    QTimer::singleShot(0, &window, [&window, path]() { window.openPath(path); });
  }

  return app.exec();
}
