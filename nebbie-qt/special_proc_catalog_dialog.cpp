#include "special_proc_catalog_dialog.hpp"

#include "path_util.hpp"

#include "nebbie/special_proc_catalog.hpp"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace nebbie::qt {

SpecialProcCatalogDialog::SpecialProcCatalogDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Catalogo special proc"));
    setMinimumWidth(560);
    setMinimumHeight(420);

    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(QStringLiteral(
        "Izanagi accetta i nomi delle special proc da tre fonti:\n"
        "1) catalogo embedded (snapshot)\n"
        "2) tabelle C++ del Server locale (specialproc_*.cpp)\n"
        "3) allowlist locale editabile qui sotto (nomi WIP / non ancora nel Server)")));

    auto* form = new QFormLayout;
    server_root_ = new QLineEdit;
    auto* browse = new QPushButton(QStringLiteral("Sfoglia…"));
    auto* server_row = new QWidget;
    auto* server_layout = new QHBoxLayout(server_row);
    server_layout->setContentsMargins(0, 0, 0, 0);
    server_layout->addWidget(server_root_, 1);
    server_layout->addWidget(browse);
    form->addRow(QStringLiteral("Root Server (NebbieArcane/Server):"), server_row);
    root->addLayout(form);

    status_label_ = new QLabel;
    status_label_->setWordWrap(true);
    root->addWidget(status_label_);

    auto* reload = new QPushButton(QStringLiteral("Aggiorna anteprima Server"));
    root->addWidget(reload);

    root->addWidget(new QLabel(QStringLiteral("Allowlist locale (un nome per riga):")));
    extra_list_ = new QListWidget;
    root->addWidget(extra_list_, 1);

    auto* extra_row = new QWidget;
    auto* extra_layout = new QHBoxLayout(extra_row);
    extra_layout->setContentsMargins(0, 0, 0, 0);
    extra_name_ = new QLineEdit;
    extra_name_->setPlaceholderText(QStringLiteral("NomeProcedura"));
    auto* add = new QPushButton(QStringLiteral("Aggiungi"));
    auto* remove = new QPushButton(QStringLiteral("Rimuovi"));
    extra_layout->addWidget(extra_name_, 1);
    extra_layout->addWidget(add);
    extra_layout->addWidget(remove);
    root->addWidget(extra_row);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    root->addWidget(buttons);

    connect(browse, &QPushButton::clicked, this, &SpecialProcCatalogDialog::browseServerRoot);
    connect(reload, &QPushButton::clicked, this, &SpecialProcCatalogDialog::reloadStatus);
    connect(add, &QPushButton::clicked, this, &SpecialProcCatalogDialog::addExtraName);
    connect(remove, &QPushButton::clicked, this, &SpecialProcCatalogDialog::removeExtraName);
    connect(extra_name_, &QLineEdit::returnPressed, this, &SpecialProcCatalogDialog::addExtraName);
    connect(buttons, &QDialogButtonBox::accepted, this, &SpecialProcCatalogDialog::acceptAndSave);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadFromCurrentCatalog();
    reloadStatus();
}

QString SpecialProcCatalogDialog::serverRoot() const {
    return server_root_->text().trimmed();
}

QStringList SpecialProcCatalogDialog::extraNames() const {
    QStringList names;
    for (int i = 0; i < extra_list_->count(); ++i) {
        names << extra_list_->item(i)->text();
    }
    return names;
}

void SpecialProcCatalogDialog::loadFromCurrentCatalog() {
    const auto config = nebbie::special_proc_catalog_config();
    if (!config.server_root.empty()) {
        server_root_->setText(qstring_from_path(config.server_root));
    } else {
        server_root_->setText(qstring_from_path(nebbie::default_nebbie_server_root()));
    }
    extra_list_->clear();
    for (const auto& name : nebbie::extra_special_proc_names()) {
        extra_list_->addItem(QString::fromStdString(name));
    }
}

void SpecialProcCatalogDialog::browseServerRoot() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Seleziona root Server"), server_root_->text());
    if (!dir.isEmpty()) {
        server_root_->setText(dir);
        reloadStatus();
    }
}

void SpecialProcCatalogDialog::reloadStatus() {
    std::size_t mob_count = 0;
    std::size_t room_count = 0;
    std::string detail;
    const bool ok = nebbie::probe_server_special_proc_tables(
        path_from_qstring(serverRoot()), &mob_count, &room_count, &detail);

    QString text;
    if (ok) {
        text = QStringLiteral("Server OK: %1 mob/obj + %2 room (%3)")
                   .arg(mob_count)
                   .arg(room_count)
                   .arg(QString::fromStdString(detail));
    } else {
        text = QStringLiteral("Server non disponibile: %1").arg(QString::fromStdString(detail));
    }
    text += QStringLiteral("\nAllowlist in dialog: %1 nomi (verranno salvati con Salva).")
                .arg(extra_list_->count());
    status_label_->setText(text);
}

void SpecialProcCatalogDialog::addExtraName() {
    const QString name = extra_name_->text().trimmed();
    if (name.isEmpty()) {
        return;
    }
    for (int i = 0; i < extra_list_->count(); ++i) {
        if (QString::compare(extra_list_->item(i)->text(), name, Qt::CaseInsensitive) == 0) {
            extra_list_->setCurrentRow(i);
            return;
        }
    }
    extra_list_->addItem(name);
    extra_name_->clear();
    reloadStatus();
}

void SpecialProcCatalogDialog::removeExtraName() {
    const auto items = extra_list_->selectedItems();
    for (QListWidgetItem* item : items) {
        delete extra_list_->takeItem(extra_list_->row(item));
    }
    reloadStatus();
}

void SpecialProcCatalogDialog::acceptAndSave() {
    saved_ = true;
    accept();
}

} // namespace nebbie::qt
