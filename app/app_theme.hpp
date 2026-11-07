#pragma once

#include <QString>

class QWidget;

enum class AppTheme {
  Dark = 0,
  Light = 1,
  ClassicGray = 2,  // grigio anni '90 / Eleutheria
};

AppTheme theme_from_key(const QString& key);
QString theme_to_key(AppTheme theme);
QString theme_label(AppTheme theme);

void apply_app_theme(QWidget* root, AppTheme theme);

AppTheme load_saved_theme();
void save_theme(AppTheme theme);
