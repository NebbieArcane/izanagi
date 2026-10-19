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

#include <algorithm>
#include <map>
#include <vector>

namespace nebbie::qt {

namespace {

const char* k_category_order[] = {
    "english_text", "room_text", "mob_text", "object_text", "room", "mob_combat",
    "mob_economy",  "reset",     "shop",     "guild",       "special", "social",
};

int category_sort_key(const std::string& category) {
    for (int i = 0; i < static_cast<int>(sizeof(k_category_order) / sizeof(k_category_order[0])); ++i) {
        if (category == k_category_order[i]) {
            return i;
        }
    }
    return 1000;
}

} // namespace

QString validation_category_title(const std::string& category) {
    if (category == "english_text") {
        return QStringLiteral("Testi inglesi (lessico)");
    }
    if (category == "room_text") {
        return QStringLiteral("Testi stanza (righe / ASCII)");
    }
    if (category == "mob_text") {
        return QStringLiteral("Testi mob (lunghezza riga)");
    }
    if (category == "object_text") {
        return QStringLiteral("Testi oggetto (lunghezza riga)");
    }
    if (category == "room") {
        return QStringLiteral("Stanze (struttura / uscite)");
    }
    if (category == "mob_combat") {
        return QStringLiteral("Combattimento mob");
    }
    if (category == "mob_economy") {
        return QStringLiteral("Economia mob (XP)");
    }
    if (category == "reset") {
        return QStringLiteral("Reset / zone");
    }
    if (category == "shop") {
        return QStringLiteral("Negozi");
    }
    if (category == "guild") {
        return QStringLiteral("Gilde");
    }
    if (category == "special") {
        return QStringLiteral("Special proc");
    }
    if (category == "social") {
        return QStringLiteral("Social");
    }
    if (category.empty()) {
        return QStringLiteral("Altro");
    }
    return QString::fromStdString(category);
}

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

    std::map<std::string, std::vector<const nebbie::ValidationIssue*>> by_category;
    for (const auto& issue : report.issues) {
        by_category[issue.category].push_back(&issue);
    }
    std::vector<std::string> categories;
    categories.reserve(by_category.size());
    for (const auto& [category, _] : by_category) {
        categories.push_back(category);
    }
    std::sort(categories.begin(), categories.end(), [](const std::string& a, const std::string& b) {
        const int ka = category_sort_key(a);
        const int kb = category_sort_key(b);
        if (ka != kb) {
            return ka < kb;
        }
        return a < b;
    });

    for (const std::string& category : categories) {
        const auto& group = by_category[category];
        lines << QStringLiteral("— %1 (%2) —")
                     .arg(validation_category_title(category))
                     .arg(static_cast<qlonglong>(group.size()));
        for (const nebbie::ValidationIssue* issue : group) {
            const char* level = issue->severity == nebbie::ValidationSeverity::error ? "ERR" : "WARN";
            lines << QStringLiteral("[%1] %2")
                         .arg(QString::fromUtf8(level))
                         .arg(QString::fromStdString(issue->message));
        }
        lines << QString();
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
