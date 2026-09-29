#include "translator_window.hpp"
#include "app_config.hpp"
#include "app_i18n.hpp"
#include "app_theme.hpp"

#include "path_util.hpp"
#include "version.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const nebbie::translate::AppConfig startup_config = nebbie::translate::read_config();
    nebbie::qt::setAppLanguage(nebbie::qt::parseLanguageCode(startup_config.ui_language));
    app.setApplicationName(nebbie::qt::cypherDisplayName());
    app.setApplicationDisplayName(nebbie::qt::cypherWindowTitle());
    app.setOrganizationName("Nebbie Arcane");
    app.setApplicationVersion(nebbie::qt::applicationVersionLabel());
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
