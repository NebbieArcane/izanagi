#include "app_theme.hpp"

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>

namespace nebbie::qt {

void applyDefaultAppTheme(QApplication& app) {
    if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
        app.setStyle(fusion);
    }

    // Classic Win95 chrome, inverted to a dark SMAUG-like desktop.
    const QColor window(64, 64, 64);          // dark "button face"
    const QColor window_text(230, 230, 230);
    const QColor base(32, 32, 32);            // tree / edits (dark "window")
    const QColor alternate_base(48, 48, 48);
    const QColor text(230, 230, 230);
    const QColor button(80, 80, 80);
    const QColor button_text(230, 230, 230);
    const QColor mid(96, 96, 96);
    const QColor dark(24, 24, 24);
    const QColor light(120, 120, 120);
    const QColor highlight(0, 0, 128);        // classic navy selection
    const QColor highlighted_text(255, 255, 255);
    const QColor placeholder(150, 150, 150);
    const QColor link(120, 160, 220);

    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, window_text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternate_base);
    palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 225));
    palette.setColor(QPalette::ToolTipText, Qt::black);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, button_text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Light, light);
    palette.setColor(QPalette::Midlight, QColor(100, 100, 100));
    palette.setColor(QPalette::Mid, mid);
    palette.setColor(QPalette::Dark, dark);
    palette.setColor(QPalette::Shadow, Qt::black);
    palette.setColor(QPalette::Link, link);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, highlighted_text);
    palette.setColor(QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::Text, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, placeholder);
    app.setPalette(palette);

    QFont ui_font(QStringLiteral("Tahoma"));
    if (!ui_font.exactMatch()) {
        ui_font = QFont(QStringLiteral("MS Shell Dlg 2"));
    }
    if (!ui_font.exactMatch()) {
        ui_font = QFont(QStringLiteral("DejaVu Sans"));
    }
    ui_font.setPointSize(9);
    app.setFont(ui_font);

    app.setStyleSheet(QStringLiteral(
        "* { font-family: Tahoma, 'MS Shell Dlg 2', 'DejaVu Sans', sans-serif; font-size: 9pt; }"
        "QMainWindow, QDialog, QWidget { background-color: #404040; color: #e6e6e6; }"
        "QMenuBar {"
        "  background-color: #404040; color: #e6e6e6;"
        "  border-bottom: 1px solid #202020;"
        "}"
        "QMenuBar::item { padding: 3px 8px; background: transparent; }"
        "QMenuBar::item:selected { background-color: #000080; color: #ffffff; }"
        "QMenu {"
        "  background-color: #404040; color: #e6e6e6;"
        "  border: 1px solid #202020;"
        "}"
        "QMenu::item:selected { background-color: #000080; color: #ffffff; }"
        "QStatusBar {"
        "  background-color: #404040;"
        "  border-top: 1px solid #787878;"
        "}"
        "QToolBar {"
        "  background-color: #404040;"
        "  border-bottom: 1px solid #202020;"
        "  spacing: 2px; padding: 2px;"
        "}"
        "QTreeWidget#navTree {"
        "  background-color: #202020;"
        "  color: #e6e6e6;"
        "  border-top: 1px solid #202020;"
        "  border-left: 1px solid #202020;"
        "  border-right: 1px solid #787878;"
        "  border-bottom: 1px solid #787878;"
        "  outline: 0;"
        "  show-decoration-selected: 1;"
        "}"
        "QTreeWidget#navTree::item { height: 18px; padding: 1px 2px; }"
        "QTreeWidget#navTree::item:selected {"
        "  background-color: #000080;"
        "  color: #ffffff;"
        "}"
        "QTreeWidget#navTree::item:hover:!selected { background-color: #505050; }"
        "QTreeWidget#navTree::branch:has-siblings:!adjoins-item {"
        "  border-image: none; border-left: 1px dotted #808080;"
        "}"
        "QTreeWidget#navTree::branch:has-siblings:adjoins-item {"
        "  border-image: none; border-left: 1px dotted #808080;"
        "}"
        "QTabWidget::pane {"
        "  background-color: #404040;"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  top: -1px;"
        "  padding: 6px;"
        "}"
        "QTabBar::tab {"
        "  background-color: #505050;"
        "  color: #e6e6e6;"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  padding: 4px 10px;"
        "  margin-right: 1px;"
        "  min-width: 4em;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #404040;"
        "  border-bottom-color: #404040;"
        "  margin-top: 0;"
        "}"
        "QTabBar::tab:!selected { margin-top: 2px; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTextEdit, QListWidget, QTableWidget,"
        "QComboBox, QComboBox QAbstractItemView {"
        "  background-color: #202020;"
        "  color: #e6e6e6;"
        "  border-top: 1px solid #202020;"
        "  border-left: 1px solid #202020;"
        "  border-right: 1px solid #787878;"
        "  border-bottom: 1px solid #787878;"
        "  selection-background-color: #000080;"
        "  selection-color: #ffffff;"
        "  padding: 2px;"
        "}"
        "QPushButton {"
        "  background-color: #505050;"
        "  color: #e6e6e6;"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  padding: 3px 10px;"
        "  min-height: 1.3em;"
        "}"
        "QPushButton:pressed {"
        "  border-top: 1px solid #202020;"
        "  border-left: 1px solid #202020;"
        "  border-right: 1px solid #787878;"
        "  border-bottom: 1px solid #787878;"
        "}"
        "QPushButton:disabled { color: #909090; }"
        "QGroupBox {"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  margin-top: 0.8em;"
        "  padding-top: 0.5em;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 8px;"
        "  padding: 0 4px;"
        "}"
        "QCheckBox, QLabel, QFormLayout QLabel { color: #e6e6e6; }"
        "QHeaderView::section {"
        "  background-color: #505050;"
        "  color: #e6e6e6;"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  padding: 3px 6px;"
        "}"
        "QScrollBar:vertical { background: #404040; width: 16px; margin: 16px 0 16px 0; }"
        "QScrollBar::handle:vertical {"
        "  background: #505050;"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  min-height: 20px;"
        "}"
        "QScrollBar:horizontal { background: #404040; height: 16px; margin: 0 16px 0 16px; }"
        "QScrollBar::handle:horizontal {"
        "  background: #505050;"
        "  border-top: 1px solid #787878;"
        "  border-left: 1px solid #787878;"
        "  border-right: 1px solid #202020;"
        "  border-bottom: 1px solid #202020;"
        "  min-width: 20px;"
        "}"
        "QSplitter::handle { background: #505050; width: 4px; height: 4px; }"
        "QScrollArea { background: #404040; border: none; }"
        "QToolTip {"
        "  background-color: #ffffe1; color: #000000;"
        "  border: 1px solid #000000;"
        "}"
        ));
}

} // namespace nebbie::qt
