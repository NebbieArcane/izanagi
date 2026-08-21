#include "mud_color_dialogs.hpp"
#include "mud_color_common.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace nebbie::qt {

namespace {

QString formatStandardCode(int color) {
    return QStringLiteral("$c%1").arg(color, 4, 10, QLatin1Char('0'));
}

} // namespace

MudColorLegendDialog::MudColorLegendDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Legenda colori MUD"));
    resize(520, 520);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral(
        "Codici colore Nebbie ($cXXXX o $$cXXXX). All'apertura i codici sono nascosti e si "
        "vede solo il testo colorato; attiva la visualizzazione estesa nelle Preferenze per "
        "mostrare anche le stringhe $cXXXX.")));

    auto* list = new QListWidget;
    for (int color = 1; color <= 15; ++color) {
        nebbie::MudColorCode code;
        code.foreground = color;
        const QString label = QStringLiteral("%1 / %2 — %3")
                                  .arg(formatStandardCode(color))
                                  .arg(QStringLiteral("$$c%1").arg(color, 4, 10, QLatin1Char('0')))
                                  .arg(QString::fromStdString(nebbie::mud_color_code_name(color)));
        auto* item = new QListWidgetItem(label);
        item->setForeground(mud_foreground_color(color, false));
        list->addItem(item);
    }

    auto* advanced = new QListWidgetItem(QStringLiteral(
        "Avanzate: cifre modificatore/grassetto/sfondo non nulle (es. $$c0109, $$c00033). "
        "Il server accetta combinazioni oltre la tabella standard."));
    advanced->setFlags(Qt::ItemIsEnabled);
    list->addItem(advanced);

    layout->addWidget(list, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);
}

MudColorInsertDialog::MudColorInsertDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Inserisci codice colore"));
    resize(420, 420);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral(
        "Scegli un colore standard da inserire alla posizione del cursore. "
        "Viene inserito come $cXXXX (puoi usare $$cXXXX manualmente in visualizzazione estesa).")));

    auto* list = new QListWidget;
    for (int color = 1; color <= 15; ++color) {
        if (color == 5) {
            continue;
        }
        const QString code = formatStandardCode(color);
        auto* item = new QListWidgetItem(
            QStringLiteral("%1 — %2").arg(code).arg(QString::fromStdString(nebbie::mud_color_code_name(color))));
        item->setData(Qt::UserRole, code);
        item->setForeground(mud_foreground_color(color, false));
        list->addItem(item);
    }
    layout->addWidget(list, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this, list]() {
        const auto* item = list->currentItem();
        if (!item) {
            return;
        }
        selected_code_ = item->data(Qt::UserRole).toString();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    if (list->count() > 0) {
        list->setCurrentRow(0);
    }
    connect(list, &QListWidget::itemDoubleClicked, this, [this, list](QListWidgetItem* item) {
        selected_code_ = item->data(Qt::UserRole).toString();
        accept();
    });
}

} // namespace nebbie::qt
