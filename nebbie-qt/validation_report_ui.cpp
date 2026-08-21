#include "validation_report_ui.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

namespace nebbie::qt {

namespace {

QString config_directory() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.config/Nebbie");
    }
    QDir().mkpath(base);
    return base;
}

QString application_log_path(const char* filename) {
    return config_directory() + QStringLiteral("/") + QString::fromUtf8(filename);
}

} // namespace

QString editor_application_log_path() {
    return application_log_path("nebbieedit.log");
}

QString translate_application_log_path() {
    return application_log_path("nebbie-translate.log");
}

QString editor_validation_log_path() {
    return application_log_path("nebbieedit-validation.log");
}

QString translate_validation_log_path() {
    return application_log_path("nebbie-translate-validation.log");
}

QString format_validation_report(const nebbie::ValidationReport& report,
                                 const QString& context,
                                 const QString& library_path) {
    QStringList lines;
    if (!context.isEmpty()) {
        lines << context;
    }
    if (!library_path.isEmpty()) {
        lines << QStringLiteral("Libreria: %1").arg(library_path);
    }
    lines << QStringLiteral("Errori: %1").arg(static_cast<qlonglong>(report.error_count()));
    lines << QStringLiteral("Avvisi: %1").arg(static_cast<qlonglong>(report.warning_count()));
    lines << QString();

    if (report.issues.empty()) {
        lines << QStringLiteral("Nessun problema rilevato.");
        return lines.join('\n');
    }

    for (const auto& issue : report.issues) {
        const char* level = issue.severity == nebbie::ValidationSeverity::error ? "ERR" : "WARN";
        lines << QStringLiteral("[%1] [%2] %3")
                     .arg(QString::fromUtf8(level))
                     .arg(QString::fromStdString(issue.category))
                     .arg(QString::fromStdString(issue.message));
    }
    return lines.join('\n');
}

QString save_validation_report_to_log(const nebbie::ValidationReport& report,
                                      const QString& log_path,
                                      const QString& context,
                                      const QString& library_path) {
    if (log_path.isEmpty()) {
        return {};
    }

    QFile file(log_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    stream << "=== " << timestamp;
    if (!context.isEmpty()) {
        stream << " — " << context;
    }
    stream << " ===\n";
    stream << format_validation_report(report, context, library_path) << "\n\n";
    return log_path;
}

void show_validation_report_dialog(QWidget* parent,
                                   const nebbie::ValidationReport& report,
                                   const QString& window_title,
                                   const QString& context,
                                   const QString& library_path,
                                   const QString& log_path) {
    const QString body = format_validation_report(report, context, library_path);
    const QString saved_log = save_validation_report_to_log(report, log_path, context, library_path);

    QDialog dialog(parent);
    dialog.setWindowTitle(window_title);
    dialog.resize(820, 520);

    auto* layout = new QVBoxLayout(&dialog);
    QString summary = QStringLiteral("Errori: %1 — Avvisi: %2")
                          .arg(static_cast<qlonglong>(report.error_count()))
                          .arg(static_cast<qlonglong>(report.warning_count()));
    if (!saved_log.isEmpty()) {
        summary += QStringLiteral("\nLog salvato in: %1").arg(saved_log);
    }
    auto* summary_label = new QLabel(summary);
    summary_label->setWordWrap(true);
    summary_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(summary_label);

    auto* text = new QPlainTextEdit;
    text->setPlainText(body);
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    layout->addWidget(text, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    auto* copy_button = buttons->addButton(QStringLiteral("Copia tutto"), QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);

    QObject::connect(copy_button, &QPushButton::clicked, &dialog, [body]() {
        QApplication::clipboard()->setText(body);
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);

    dialog.exec();
}

} // namespace nebbie::qt
