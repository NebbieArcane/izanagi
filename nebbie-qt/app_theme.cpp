#include "app_theme.hpp"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

namespace nebbie::qt {

void applyDefaultAppTheme(QApplication& app) {
    if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        app.setStyle(fusion);
    }

    QPalette palette;
    const QColor window(53, 53, 53);
    const QColor window_text(230, 230, 230);
    const QColor base(32, 32, 32);
    const QColor alternate_base(45, 45, 45);
    const QColor text(230, 230, 230);
    const QColor button(53, 53, 53);
    const QColor button_text(230, 230, 230);
    const QColor mid(70, 70, 70);
    const QColor dark(25, 25, 25);
    const QColor highlight(42, 130, 218);
    const QColor placeholder(127, 127, 127);
    const QColor link(42, 130, 218);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, window_text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternate_base);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, button_text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, link);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::black);
    palette.setColor(QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Mid, mid);
    palette.setColor(QPalette::Dark, dark);
    palette.setColor(QPalette::Shadow, dark);
    palette.setColor(QPalette::Disabled, QPalette::Text, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, placeholder);

    app.setPalette(palette);
}

} // namespace nebbie::qt
