#pragma once

#include "nebbie/validate.hpp"

#include <QString>

class QWidget;

namespace nebbie::qt {

QString editor_application_log_path();
QString translate_application_log_path();
QString editor_validation_log_path();
QString translate_validation_log_path();

QString format_validation_report(const nebbie::ValidationReport& report,
                                 const QString& context = {},
                                 const QString& library_path = {});

QString save_validation_report_to_log(const nebbie::ValidationReport& report,
                                      const QString& log_path,
                                      const QString& context = {},
                                      const QString& library_path = {});

void show_validation_report_dialog(QWidget* parent,
                                   const nebbie::ValidationReport& report,
                                   const QString& window_title,
                                   const QString& context = {},
                                   const QString& library_path = {},
                                   const QString& log_path = {});

} // namespace nebbie::qt
