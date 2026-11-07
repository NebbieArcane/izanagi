#include "app_theme.hpp"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QWidget>

namespace {

constexpr const char* kOrg = "NebbieArcane";
constexpr const char* kApp = "Eleutheria";
constexpr const char* kThemeKey = "ui/theme";

QPalette dark_palette() {
  QPalette p;
  p.setColor(QPalette::Window, QColor(45, 45, 48));
  p.setColor(QPalette::WindowText, QColor(220, 220, 220));
  p.setColor(QPalette::Base, QColor(30, 30, 30));
  p.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
  p.setColor(QPalette::ToolTipBase, QColor(45, 45, 48));
  p.setColor(QPalette::ToolTipText, QColor(220, 220, 220));
  p.setColor(QPalette::Text, QColor(220, 220, 220));
  p.setColor(QPalette::Button, QColor(55, 55, 58));
  p.setColor(QPalette::ButtonText, QColor(220, 220, 220));
  p.setColor(QPalette::BrightText, QColor(255, 80, 80));
  p.setColor(QPalette::Highlight, QColor(42, 130, 218));
  p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  p.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));
  p.setColor(QPalette::Link, QColor(90, 160, 255));
  return p;
}

QPalette light_palette() {
  QPalette p;
  p.setColor(QPalette::Window, QColor(245, 245, 245));
  p.setColor(QPalette::WindowText, QColor(20, 20, 20));
  p.setColor(QPalette::Base, QColor(255, 255, 255));
  p.setColor(QPalette::AlternateBase, QColor(238, 238, 238));
  p.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
  p.setColor(QPalette::ToolTipText, QColor(20, 20, 20));
  p.setColor(QPalette::Text, QColor(20, 20, 20));
  p.setColor(QPalette::Button, QColor(232, 232, 232));
  p.setColor(QPalette::ButtonText, QColor(20, 20, 20));
  p.setColor(QPalette::BrightText, QColor(200, 0, 0));
  p.setColor(QPalette::Highlight, QColor(0, 120, 215));
  p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  p.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
  p.setColor(QPalette::Link, QColor(0, 102, 204));
  return p;
}

QPalette classic_palette() {
  // Win95 / Delphi Eleutheria: #d4d0c8
  QPalette p;
  p.setColor(QPalette::Window, QColor(0xd4, 0xd0, 0xc8));
  p.setColor(QPalette::WindowText, QColor(0, 0, 0));
  p.setColor(QPalette::Base, QColor(255, 255, 255));
  p.setColor(QPalette::AlternateBase, QColor(0xe8, 0xe4, 0xdc));
  p.setColor(QPalette::ToolTipBase, QColor(255, 255, 225));
  p.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
  p.setColor(QPalette::Text, QColor(0, 0, 0));
  p.setColor(QPalette::Button, QColor(0xd4, 0xd0, 0xc8));
  p.setColor(QPalette::ButtonText, QColor(0, 0, 0));
  p.setColor(QPalette::BrightText, QColor(255, 255, 255));
  p.setColor(QPalette::Highlight, QColor(0x0a, 0x24, 0x6a));
  p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  p.setColor(QPalette::PlaceholderText, QColor(100, 100, 100));
  p.setColor(QPalette::Link, QColor(0x0a, 0x24, 0x6a));
  p.setColor(QPalette::Light, QColor(255, 255, 255));
  p.setColor(QPalette::Midlight, QColor(0xe0, 0xdc, 0xd4));
  p.setColor(QPalette::Dark, QColor(0x80, 0x80, 0x80));
  p.setColor(QPalette::Mid, QColor(0xa0, 0xa0, 0xa0));
  p.setColor(QPalette::Shadow, QColor(0x40, 0x40, 0x40));
  return p;
}

const char* dark_qss() {
  return R"(
    QToolTip { color: #ddd; background: #2d2d30; border: 1px solid #555; }
    QMenuBar { background: #2d2d30; }
    QMenuBar::item:selected { background: #3e3e42; }
    QMenu { background: #2d2d30; border: 1px solid #555; }
    QMenu::item:selected { background: #094771; }
    QToolBar { background: #2d2d30; border-bottom: 1px solid #3e3e42; spacing: 4px; }
    QTabWidget::pane { border: 1px solid #3e3e42; }
    QTabBar::tab { background: #2d2d30; color: #ccc; padding: 6px 12px; border: 1px solid #3e3e42; }
    QTabBar::tab:selected { background: #1e1e1e; color: #fff; }
    QHeaderView::section { background: #2d2d30; color: #ccc; border: 1px solid #3e3e42; padding: 4px; }
    QStatusBar { background: #007acc; color: #fff; }
    QDockWidget::title { background: #2d2d30; padding: 4px; }
  )";
}

const char* light_qss() {
  return R"(
    QToolTip { color: #222; background: #ffffe1; border: 1px solid #888; }
    QMenuBar { background: #f5f5f5; }
    QMenuBar::item:selected { background: #cce4f7; }
    QMenu { background: #ffffff; border: 1px solid #aaa; }
    QMenu::item:selected { background: #0078d7; color: #fff; }
    QToolBar { background: #f0f0f0; border-bottom: 1px solid #ccc; spacing: 4px; }
    QTabWidget::pane { border: 1px solid #bbb; }
    QTabBar::tab { background: #e8e8e8; color: #333; padding: 6px 12px; border: 1px solid #bbb; }
    QTabBar::tab:selected { background: #ffffff; color: #000; }
    QHeaderView::section { background: #e8e8e8; color: #333; border: 1px solid #bbb; padding: 4px; }
    QStatusBar { background: #0078d7; color: #fff; }
    QDockWidget::title { background: #e8e8e8; padding: 4px; }
  )";
}

const char* classic_qss() {
  return R"(
    QMainWindow, QWidget { background: #d4d0c8; color: #000000; }
    QToolTip { color: #000; background: #ffffe1; border: 1px solid #000; }
    QMenuBar { background: #d4d0c8; }
    QMenuBar::item:selected { background: #0a246a; color: white; }
    QMenu { background: #d4d0c8; border: 1px solid #808080; }
    QMenu::item:selected { background: #0a246a; color: white; }
    QToolBar { background: #d4d0c8; border: 1px solid #808080; spacing: 4px; }
    QListWidget, QPlainTextEdit, QLineEdit, QSpinBox, QComboBox, QTableWidget, QTreeWidget {
      background: #ffffff; border: 1px solid #808080; color: #000;
    }
    QTabWidget::pane { border: 1px solid #808080; background: #d4d0c8; }
    QTabBar::tab { background: #d4d0c8; padding: 6px 12px; border: 1px solid #808080; color: #000; }
    QTabBar::tab:selected { background: #ffffff; }
    QHeaderView::section { background: #d4d0c8; color: #000; border: 1px solid #808080; padding: 4px; }
    QStatusBar { background: #d4d0c8; color: #000; border-top: 1px solid #808080; }
    QDockWidget::title { background: #d4d0c8; padding: 4px; border: 1px solid #808080; }
    QPushButton { background: #d4d0c8; border: 1px solid #808080; padding: 3px 10px; }
    QPushButton:pressed { background: #c0bcb4; }
    QCheckBox, QLabel, QRadioButton { color: #000; background: transparent; }
  )";
}

}  // namespace

AppTheme theme_from_key(const QString& key) {
  if(key == QLatin1String("light")) {
    return AppTheme::Light;
  }
  if(key == QLatin1String("classic") || key == QLatin1String("gray") ||
     key == QLatin1String("eleutheria")) {
    return AppTheme::ClassicGray;
  }
  return AppTheme::Dark;
}

QString theme_to_key(AppTheme theme) {
  switch(theme) {
    case AppTheme::Light:
      return QStringLiteral("light");
    case AppTheme::ClassicGray:
      return QStringLiteral("classic");
    case AppTheme::Dark:
    default:
      return QStringLiteral("dark");
  }
}

QString theme_label(AppTheme theme) {
  switch(theme) {
    case AppTheme::Light:
      return QStringLiteral("Chiaro");
    case AppTheme::ClassicGray:
      return QStringLiteral("Grigio anni '90 (Eleutheria)");
    case AppTheme::Dark:
    default:
      return QStringLiteral("Scuro");
  }
}

void apply_app_theme(QWidget* root, AppTheme theme) {
  qApp->setStyle(QStringLiteral("Fusion"));
  QPalette palette;
  const char* qss = dark_qss();
  switch(theme) {
    case AppTheme::Light:
      palette = light_palette();
      qss = light_qss();
      break;
    case AppTheme::ClassicGray:
      palette = classic_palette();
      qss = classic_qss();
      break;
    case AppTheme::Dark:
    default:
      palette = dark_palette();
      qss = dark_qss();
      break;
  }
  qApp->setPalette(palette);
  if(root != nullptr) {
    root->setStyleSheet(QString::fromUtf8(qss));
  } else {
    qApp->setStyleSheet(QString::fromUtf8(qss));
  }
}

AppTheme load_saved_theme() {
  QSettings settings(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
  return theme_from_key(settings.value(QString::fromUtf8(kThemeKey), QStringLiteral("dark")).toString());
}

void save_theme(AppTheme theme) {
  QSettings settings(QString::fromUtf8(kOrg), QString::fromUtf8(kApp));
  settings.setValue(QString::fromUtf8(kThemeKey), theme_to_key(theme));
}
