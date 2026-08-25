#include "translator_window.hpp"
#include "app_config.hpp"
#include "app_theme.hpp"

#include "path_util.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Cypher");
    app.setApplicationDisplayName("Cypher");
    app.setOrganizationName("Nebbie Arcane");
    nebbie::qt::applyDefaultAppTheme(app);
    const QIcon app_icon(QStringLiteral(":/app-icon.png"));
    app.setWindowIcon(app_icon);

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
