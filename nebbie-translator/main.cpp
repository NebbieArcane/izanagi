#include "translator_window.hpp"
#include "app_config.hpp"

#include "path_util.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Nebbie Translate");
    app.setApplicationDisplayName("Nebbie Translate");
    app.setOrganizationName("Nebbie Arcane");

    TranslatorWindow window;
    window.show();

    const QStringList args = QCoreApplication::arguments();
    if (args.size() >= 2) {
        window.openLibPath(args.at(1));
    } else {
        QTimer::singleShot(0, &window, &TranslatorWindow::openStartupLib);
    }

    return app.exec();
}
