#include "translator_room_widget.hpp"

#include "mud_color_widgets.hpp"
#include "nebbie/mud_text.hpp"
#include "nebbie/room_catalog.hpp"
#include "nebbie/text_lines.hpp"
#include "room_text_monitors.hpp"

#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QLabel* makeLineInfoLabel() {
    return nebbie::qt::makeTextMonitorLabel();
}

nebbie::Exit readExitItem(const QListWidgetItem* item) {
    nebbie::Exit exit;
    exit.direction = item->data(Qt::UserRole).toInt();
    exit.to_room = item->data(Qt::UserRole + 1).toLongLong();
    exit.description = item->data(Qt::UserRole + 2).toString().toStdString();
    exit.keyword = item->data(Qt::UserRole + 3).toString().toStdString();
    exit.exit_info = item->data(Qt::UserRole + 4).toLongLong();
    exit.key = item->data(Qt::UserRole + 5).toLongLong();
    exit.open_cmd = item->data(Qt::UserRole + 6).toLongLong();
    return exit;
}

void writeExitItem(QListWidgetItem* item, const nebbie::Exit& exit) {
    item->setData(Qt::UserRole, exit.direction);
    item->setData(Qt::UserRole + 1, static_cast<qlonglong>(exit.to_room));
    item->setData(Qt::UserRole + 2, QString::fromStdString(exit.description));
    item->setData(Qt::UserRole + 3, QString::fromStdString(exit.keyword));
    item->setData(Qt::UserRole + 4, static_cast<qlonglong>(exit.exit_info));
    item->setData(Qt::UserRole + 5, static_cast<qlonglong>(exit.key));
    item->setData(Qt::UserRole + 6, static_cast<qlonglong>(exit.open_cmd));

    item->setText(QString("%1 -> #%2")
                      .arg(QString::fromStdString(nebbie::exit_direction_label(exit.direction)))
                      .arg(exit.to_room));
}

bool textHasLineLengthIssues(const std::string& text, int max_length) {
    if (max_length <= 0) {
        return false;
    }
    return !nebbie::check_visible_text_line_lengths(text, max_length).ok();
}

QFrame* makeSectionSeparator() {
    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

} // namespace

TranslatorRoomWidget::TranslatorRoomWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    root->addWidget(new QLabel(
        "Nome e descrizione principale della stanza (testo mostrato all'ingresso).\n"
        "Il client di gioco legge solo testo ASCII: usa e', a', o', u' al posto delle lettere accentate."));

    name_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(name_);
    name_line_info_ = makeLineInfoLabel();
    name_ascii_info_ = makeLineInfoLabel();
    description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudDescriptionField(description_);
    description_line_info_ = makeLineInfoLabel();
    description_ascii_info_ = makeLineInfoLabel();

    auto* text_form = new QFormLayout;
    text_form->addRow("Nome:", name_);
    text_form->addRow("", name_line_info_);
    text_form->addRow("", name_ascii_info_);
    text_form->addRow("Descrizione:", description_);
    text_form->addRow("", description_line_info_);
    text_form->addRow("", description_ascii_info_);
    root->addLayout(text_form);

    auto* text_buttons = new QHBoxLayout;
    auto* text_accents = new QPushButton("Sostituisci accenti (nome e descrizione)");
    text_buttons->addWidget(text_accents);
    text_buttons->addStretch();
    root->addLayout(text_buttons);

    root->addWidget(makeSectionSeparator());
    root->addWidget(new QLabel("Descrizioni extra attivate da keyword (sezione E in myst.wld)."));

    extra_desc_list_ = new QListWidget;
    extra_desc_list_->setMaximumHeight(140);
    extra_desc_keyword_ = new QLineEdit;
    extra_desc_description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudDescriptionField(extra_desc_description_);
    extra_desc_description_->setMinimumHeight(160);
    extra_desc_line_info_ = makeLineInfoLabel();
    extra_desc_ascii_info_ = makeLineInfoLabel();

    auto* extra_form = new QFormLayout;
    extra_form->addRow("Keyword:", extra_desc_keyword_);
    extra_form->addRow("Descrizione:", extra_desc_description_);
    extra_form->addRow("", extra_desc_line_info_);
    extra_form->addRow("", extra_desc_ascii_info_);
    auto* extra_buttons = new QHBoxLayout;
    auto* extra_add = new QPushButton("Aggiungi / aggiorna");
    auto* extra_remove = new QPushButton("Rimuovi");
    auto* extra_accents = new QPushButton("Sostituisci accenti (extra)");
    extra_buttons->addWidget(extra_add);
    extra_buttons->addWidget(extra_remove);
    extra_buttons->addWidget(extra_accents);
    extra_buttons->addStretch();
    root->addWidget(extra_desc_list_);
    root->addLayout(extra_form);
    root->addLayout(extra_buttons);

    root->addWidget(makeSectionSeparator());
    root->addWidget(new QLabel(
        "Testo di look delle uscite (Description in myst.wld). Direzione e destinazione sono in sola lettura."));

    exit_list_ = new QListWidget;
    exit_list_->setMaximumHeight(160);
    exit_info_ = new QLabel("(nessuna uscita selezionata)");
    exit_info_->setWordWrap(true);
    exit_description_ = new nebbie::qt::MudColorTextEdit;
    nebbie::qt::configureMudSingleLineField(exit_description_);
    exit_line_info_ = makeLineInfoLabel();
    exit_ascii_info_ = makeLineInfoLabel();

    auto* exit_form = new QFormLayout;
    exit_form->addRow("Uscita:", exit_info_);
    exit_form->addRow("Description (look):", exit_description_);
    exit_form->addRow("", exit_line_info_);
    exit_form->addRow("", exit_ascii_info_);
    auto* exit_buttons = new QHBoxLayout;
    auto* exit_apply = new QPushButton("Applica description uscita");
    auto* exit_accents = new QPushButton("Sostituisci accenti (uscite)");
    exit_buttons->addWidget(exit_apply);
    exit_buttons->addWidget(exit_accents);
    exit_buttons->addStretch();
    root->addWidget(exit_list_);
    root->addLayout(exit_form);
    root->addLayout(exit_buttons);
    root->addStretch();

    connect(extra_desc_list_, &QListWidget::currentRowChanged, this, [this](int) { onExtraDescSelected(); });
    connect(extra_add, &QPushButton::clicked, this, &TranslatorRoomWidget::addExtraDesc);
    connect(extra_remove, &QPushButton::clicked, this, &TranslatorRoomWidget::removeExtraDesc);
    connect(exit_list_, &QListWidget::currentRowChanged, this, [this](int) { onExitSelected(); });
    connect(exit_apply, &QPushButton::clicked, this, &TranslatorRoomWidget::applyExitDescription);
    connect(text_accents, &QPushButton::clicked, this, &TranslatorRoomWidget::convertAccentsOnTextSection);
    connect(extra_accents, &QPushButton::clicked, this, &TranslatorRoomWidget::convertAccentsOnExtraSection);
    connect(exit_accents, &QPushButton::clicked, this, &TranslatorRoomWidget::convertAccentsOnExitSection);

    hookMudField(name_);
    hookMudField(description_);
    hookMudField(extra_desc_description_);
    hookMudField(exit_description_);
}

void TranslatorRoomWidget::hookMudField(nebbie::qt::MudColorTextEdit* field) {
    connect(field, &nebbie::qt::MudColorTextEdit::storageTextChanged, this,
            &TranslatorRoomWidget::updateLineLengthIndicators);
}

void TranslatorRoomWidget::setMaxLineLength(int max_length) {
    max_line_length_ = max_length < 0 ? 0 : max_length;
    applyColorSettingsToFields();
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::setShowColorCodes(bool show) {
    show_color_codes_ = show;
    applyColorSettingsToFields();
}

void TranslatorRoomWidget::applyColorSettingsToFields() {
    const auto apply = [this](nebbie::qt::MudColorTextEdit* field) {
        field->setShowColorCodes(show_color_codes_);
        field->setMaxLineLength(max_line_length_);
    };
    apply(name_);
    apply(description_);
    apply(extra_desc_description_);
    apply(exit_description_);
}

nebbie::qt::MudColorTextEdit* TranslatorRoomWidget::focusedMudField() const {
    const auto widgets = std::initializer_list<nebbie::qt::MudColorTextEdit*>{
        name_, description_, extra_desc_description_, exit_description_};
    for (auto* field : widgets) {
        if (field && field->hasFocus()) {
            return field;
        }
    }
    return description_;
}

void TranslatorRoomWidget::updateLineLengthIndicators() {
    if (loading_) {
        return;
    }
    nebbie::qt::updateLineLengthMonitor(name_line_info_, name_, max_line_length_);
    nebbie::qt::updateLineLengthMonitor(description_line_info_, description_, max_line_length_);
    nebbie::qt::updateLineLengthMonitor(extra_desc_line_info_, extra_desc_description_, max_line_length_);
    nebbie::qt::updateLineLengthMonitor(exit_line_info_, exit_description_, max_line_length_);

    nebbie::qt::updateAsciiMonitor(name_ascii_info_, name_->storageText().toStdString());
    nebbie::qt::updateAsciiMonitor(description_ascii_info_, description_->storageText().toStdString());
    nebbie::qt::updateAsciiMonitor(extra_desc_ascii_info_, extra_desc_description_->storageText().toStdString());
    nebbie::qt::updateAsciiMonitor(exit_ascii_info_, exit_description_->storageText().toStdString());
}

namespace {

QString convertAccents(const QString& text) {
    return QString::fromStdString(nebbie::transliterate_italian_accents_to_apostrophe(text.toStdString()));
}

} // namespace

void TranslatorRoomWidget::convertAccentsOnTextSection() {
    name_->setStorageText(convertAccents(name_->storageText()));
    description_->setStorageText(convertAccents(description_->storageText()));
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::convertAccentsOnExtraSection() {
    extra_desc_keyword_->setText(convertAccents(extra_desc_keyword_->text()));
    extra_desc_description_->setStorageText(convertAccents(extra_desc_description_->storageText()));
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::convertAccentsOnExitSection() {
    exit_description_->setStorageText(convertAccents(exit_description_->storageText()));
    updateLineLengthIndicators();
}

bool TranslatorRoomWidget::currentFieldsHaveLineLengthIssues() const {
    if (max_line_length_ <= 0) {
        return false;
    }

    if (textHasLineLengthIssues(name_->storageText().toStdString(), max_line_length_)) {
        return true;
    }
    if (textHasLineLengthIssues(description_->storageText().toStdString(), max_line_length_)) {
        return true;
    }
    if (textHasLineLengthIssues(extra_desc_description_->storageText().toStdString(), max_line_length_)) {
        return true;
    }
    if (textHasLineLengthIssues(exit_description_->storageText().toStdString(), max_line_length_)) {
        return true;
    }

    for (int i = 0; i < extra_desc_list_->count(); ++i) {
        const auto* item = extra_desc_list_->item(i);
        if (textHasLineLengthIssues(item->data(Qt::UserRole + 1).toString().toStdString(), max_line_length_)) {
            return true;
        }
    }

    for (int i = 0; i < exit_list_->count(); ++i) {
        const nebbie::Exit exit = readExitItem(exit_list_->item(i));
        if (textHasLineLengthIssues(exit.description, max_line_length_)) {
            return true;
        }
    }

    return false;
}

void TranslatorRoomWidget::loadFromRoom(const nebbie::Room& room) {
    loading_ = true;
    name_->setStorageText(QString::fromStdString(room.name));
    description_->setStorageText(QString::fromStdString(room.description));

    extra_desc_list_->clear();
    for (const auto& extra : room.extra_descs) {
        const QString label = QString::fromStdString(extra.keyword);
        auto* item = new QListWidgetItem(label.isEmpty() ? "(no keyword)" : label);
        item->setData(Qt::UserRole, QString::fromStdString(extra.keyword));
        item->setData(Qt::UserRole + 1, QString::fromStdString(extra.description));
        extra_desc_list_->addItem(item);
    }

    exit_list_->clear();
    for (const auto& exit : room.exits) {
        auto* item = new QListWidgetItem;
        writeExitItem(item, exit);
        exit_list_->addItem(item);
    }

    loading_ = false;
    refreshExtraDescForm();
    refreshExitForm();
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::saveTranslatableFields(const nebbie::Room& original, nebbie::Room& room) const {
    room.name = name_->storageText().toStdString();
    room.description = description_->storageText().toStdString();

    room.extra_descs.clear();
    for (int i = 0; i < extra_desc_list_->count(); ++i) {
        const auto* item = extra_desc_list_->item(i);
        nebbie::ExtraDesc extra;
        extra.keyword = item->data(Qt::UserRole).toString().toStdString();
        extra.description = item->data(Qt::UserRole + 1).toString().toStdString();
        room.extra_descs.push_back(std::move(extra));
    }

    room.exits = original.exits;
    for (int i = 0; i < exit_list_->count(); ++i) {
        const nebbie::Exit edited = readExitItem(exit_list_->item(i));
        for (auto& exit : room.exits) {
            if (exit.direction == edited.direction) {
                exit.description = edited.description;
                break;
            }
        }
    }
}

void TranslatorRoomWidget::onExtraDescSelected() {
    refreshExtraDescForm();
}

void TranslatorRoomWidget::addExtraDesc() {
    const QString keyword = extra_desc_keyword_->text();
    const QString description = extra_desc_description_->storageText();
    if (keyword.trimmed().isEmpty() && description.trimmed().isEmpty()) {
        return;
    }

    for (int i = 0; i < extra_desc_list_->count(); ++i) {
        auto* item = extra_desc_list_->item(i);
        if (item->data(Qt::UserRole).toString() == keyword) {
            item->setData(Qt::UserRole + 1, description);
            item->setText(keyword.isEmpty() ? "(no keyword)" : keyword);
            updateLineLengthIndicators();
            return;
        }
    }

    auto* item = new QListWidgetItem(keyword.isEmpty() ? "(no keyword)" : keyword);
    item->setData(Qt::UserRole, keyword);
    item->setData(Qt::UserRole + 1, description);
    extra_desc_list_->addItem(item);
    extra_desc_list_->setCurrentItem(item);
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::removeExtraDesc() {
    delete extra_desc_list_->takeItem(extra_desc_list_->currentRow());
    refreshExtraDescForm();
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::refreshExtraDescForm() {
    if (loading_) {
        return;
    }
    const auto* item = extra_desc_list_->currentItem();
    if (!item) {
        extra_desc_keyword_->clear();
        extra_desc_description_->setStorageText({});
        updateLineLengthIndicators();
        return;
    }
    extra_desc_keyword_->setText(item->data(Qt::UserRole).toString());
    extra_desc_description_->setStorageText(item->data(Qt::UserRole + 1).toString());
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::onExitSelected() {
    refreshExitForm();
}

void TranslatorRoomWidget::applyExitDescription() {
    auto* item = exit_list_->currentItem();
    if (!item) {
        return;
    }
    nebbie::Exit exit = readExitItem(item);
    exit.description = exit_description_->storageText().toStdString();
    writeExitItem(item, exit);
    updateLineLengthIndicators();
}

void TranslatorRoomWidget::refreshExitForm() {
    if (loading_) {
        return;
    }
    const auto* item = exit_list_->currentItem();
    if (!item) {
        exit_info_->setText("(nessuna uscita selezionata)");
        exit_description_->setStorageText({});
        updateLineLengthIndicators();
        return;
    }
    const nebbie::Exit exit = readExitItem(item);
    exit_info_->setText(QString("%1 -> stanza #%2")
                            .arg(QString::fromStdString(nebbie::exit_direction_label(exit.direction)))
                            .arg(exit.to_room));
    exit_description_->setStorageText(QString::fromStdString(exit.description));
    updateLineLengthIndicators();
}
