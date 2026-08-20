#include "translator_room_widget.hpp"

#include "nebbie/room_catalog.hpp"
#include "nebbie/text_lines.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QFont>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

void configureDescriptionField(QTextEdit* field) {
    field->setMinimumWidth(560);
    field->setMinimumHeight(320);
    field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    field->setLineWrapMode(QTextEdit::NoWrap);
    field->setAcceptRichText(false);
    QFont font = field->font();
    font.setFamily(QStringLiteral("Monospace"));
    font.setStyleHint(QFont::Monospace);
    field->setFont(font);
}

QLabel* makeLineInfoLabel() {
    auto* label = new QLabel;
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
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
    return !nebbie::check_text_line_lengths(text, max_length).ok();
}

} // namespace

TranslatorRoomWidget::TranslatorRoomWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    tabs_ = new QTabWidget;

    auto* text_tab = new QWidget;
    auto* text_layout = new QVBoxLayout(text_tab);
    text_layout->addWidget(new QLabel(
        "Nome e descrizione principale della stanza (testo mostrato all'ingresso)."));
    name_ = new QLineEdit;
    name_->setMinimumWidth(420);
    name_line_info_ = makeLineInfoLabel();
    description_ = new QTextEdit;
    configureDescriptionField(description_);
    description_line_info_ = makeLineInfoLabel();
    auto* text_form = new QFormLayout;
    text_form->addRow("Nome:", name_);
    text_form->addRow("", name_line_info_);
    text_form->addRow("Descrizione:", description_);
    text_form->addRow("", description_line_info_);
    text_layout->addLayout(text_form);
    tabs_->addTab(text_tab, "Testo");

    auto* extra_tab = new QWidget;
    auto* extra_layout = new QVBoxLayout(extra_tab);
    extra_layout->addWidget(new QLabel(
        "Descrizioni extra attivate da keyword (sezione E in myst.wld)."));
    extra_desc_list_ = new QListWidget;
    extra_desc_list_->setMaximumHeight(140);
    extra_desc_keyword_ = new QLineEdit;
    extra_desc_description_ = new QTextEdit;
    configureDescriptionField(extra_desc_description_);
    extra_desc_description_->setMinimumHeight(160);
    extra_desc_line_info_ = makeLineInfoLabel();
    auto* extra_form = new QFormLayout;
    extra_form->addRow("Keyword:", extra_desc_keyword_);
    extra_form->addRow("Descrizione:", extra_desc_description_);
    extra_form->addRow("", extra_desc_line_info_);
    auto* extra_buttons = new QHBoxLayout;
    auto* extra_add = new QPushButton("Aggiungi / aggiorna");
    auto* extra_remove = new QPushButton("Rimuovi");
    extra_buttons->addWidget(extra_add);
    extra_buttons->addWidget(extra_remove);
    extra_buttons->addStretch();
    extra_layout->addWidget(extra_desc_list_);
    extra_layout->addLayout(extra_form);
    extra_layout->addLayout(extra_buttons);
    tabs_->addTab(extra_tab, "Descrizioni extra");

    auto* exit_tab = new QWidget;
    auto* exit_layout = new QVBoxLayout(exit_tab);
    exit_layout->addWidget(new QLabel(
        "Testo di look delle uscite (Description in myst.wld). Direzione e destinazione sono in sola lettura."));
    exit_list_ = new QListWidget;
    exit_list_->setMaximumHeight(160);
    exit_info_ = new QLabel("(nessuna uscita selezionata)");
    exit_info_->setWordWrap(true);
    exit_description_ = new QLineEdit;
    exit_description_->setMinimumWidth(420);
    exit_line_info_ = makeLineInfoLabel();
    auto* exit_form = new QFormLayout;
    exit_form->addRow("Uscita:", exit_info_);
    exit_form->addRow("Description (look):", exit_description_);
    exit_form->addRow("", exit_line_info_);
    auto* exit_buttons = new QHBoxLayout;
    auto* exit_apply = new QPushButton("Applica description uscita");
    exit_buttons->addWidget(exit_apply);
    exit_buttons->addStretch();
    exit_layout->addWidget(exit_list_);
    exit_layout->addLayout(exit_form);
    exit_layout->addLayout(exit_buttons);
    tabs_->addTab(exit_tab, "Uscite");

    root->addWidget(tabs_);

    connect(extra_desc_list_, &QListWidget::currentRowChanged, this, [this](int) { onExtraDescSelected(); });
    connect(extra_add, &QPushButton::clicked, this, &TranslatorRoomWidget::addExtraDesc);
    connect(extra_remove, &QPushButton::clicked, this, &TranslatorRoomWidget::removeExtraDesc);
    connect(exit_list_, &QListWidget::currentRowChanged, this, [this](int) { onExitSelected(); });
    connect(exit_apply, &QPushButton::clicked, this, &TranslatorRoomWidget::applyExitDescription);

    const auto hook_text_field = [this](QTextEdit* field) {
        connect(field, &QTextEdit::textChanged, this, &TranslatorRoomWidget::updateLineLengthIndicators);
        connect(field, &QTextEdit::cursorPositionChanged, this, &TranslatorRoomWidget::updateLineLengthIndicators);
    };
    hook_text_field(description_);
    hook_text_field(extra_desc_description_);
    connect(name_, &QLineEdit::textChanged, this, &TranslatorRoomWidget::updateLineLengthIndicators);
    connect(exit_description_, &QLineEdit::textChanged, this, &TranslatorRoomWidget::updateLineLengthIndicators);
}

void TranslatorRoomWidget::setMaxLineLength(int max_length) {
    max_line_length_ = max_length < 0 ? 0 : max_length;
    updateLineLengthIndicators();
}

QString TranslatorRoomWidget::formatMultilineStatus(const QString& text, const QTextEdit* field) const {
    if (max_line_length_ <= 0) {
        return QStringLiteral("Conteggio righe attivo solo con un limite impostato (Preferenze).");
    }

    const QStringList lines = text.split('\n');
    const int current_line = field->textCursor().blockNumber() + 1;
    const int current_length =
        (current_line >= 1 && current_line <= lines.size()) ? lines[current_line - 1].length() : 0;

    QString status = QString("Riga %1: %2/%3 caratteri")
                         .arg(current_line)
                         .arg(current_length)
                         .arg(max_line_length_);

    QStringList overlong;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].length() > max_line_length_) {
            overlong << QString("riga %1 (%2)").arg(i + 1).arg(lines[i].length());
        }
    }
    if (!overlong.isEmpty()) {
        status += QStringLiteral(" — oltre limite: ") + overlong.join(", ");
    }
    return status;
}

void TranslatorRoomWidget::updateSingleLineIndicator(QLineEdit* field, QLabel* label) const {
    if (max_line_length_ <= 0) {
        label->clear();
        label->setStyleSheet({});
        return;
    }

    const int length = field->text().length();
    label->setText(QString("%1/%2 caratteri").arg(length).arg(max_line_length_));
    if (length > max_line_length_) {
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else {
        label->setStyleSheet(QStringLiteral("color: #406040;"));
    }
}

void TranslatorRoomWidget::updateMultilineIndicator(QTextEdit* field, QLabel* label) const {
    if (max_line_length_ <= 0) {
        label->clear();
        label->setStyleSheet({});
        return;
    }

    const QString status = formatMultilineStatus(field->toPlainText(), field);
    label->setText(status);

    const nebbie::TextLineLengthReport report =
        nebbie::check_text_line_lengths(field->toPlainText().toStdString(), max_line_length_);
    if (!report.ok()) {
        label->setStyleSheet(QStringLiteral("color: #b03000; font-weight: bold;"));
    } else {
        label->setStyleSheet(QStringLiteral("color: #406040;"));
    }
}

void TranslatorRoomWidget::updateLineLengthIndicators() {
    if (loading_) {
        return;
    }
    updateSingleLineIndicator(name_, name_line_info_);
    updateMultilineIndicator(description_, description_line_info_);
    updateMultilineIndicator(extra_desc_description_, extra_desc_line_info_);
    updateSingleLineIndicator(exit_description_, exit_line_info_);
}

bool TranslatorRoomWidget::currentFieldsHaveLineLengthIssues() const {
    if (max_line_length_ <= 0) {
        return false;
    }

    if (textHasLineLengthIssues(name_->text().toStdString(), max_line_length_)) {
        return true;
    }
    if (textHasLineLengthIssues(description_->toPlainText().toStdString(), max_line_length_)) {
        return true;
    }
    if (textHasLineLengthIssues(extra_desc_description_->toPlainText().toStdString(), max_line_length_)) {
        return true;
    }
    if (textHasLineLengthIssues(exit_description_->text().toStdString(), max_line_length_)) {
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
    name_->setText(QString::fromStdString(room.name));
    description_->setPlainText(QString::fromStdString(room.description));

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
    room.name = name_->text().toStdString();
    room.description = description_->toPlainText().toStdString();

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
    const QString description = extra_desc_description_->toPlainText();
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
        extra_desc_description_->clear();
        updateLineLengthIndicators();
        return;
    }
    extra_desc_keyword_->setText(item->data(Qt::UserRole).toString());
    extra_desc_description_->setPlainText(item->data(Qt::UserRole + 1).toString());
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
    exit.description = exit_description_->text().toStdString();
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
        exit_description_->clear();
        updateLineLengthIndicators();
        return;
    }
    const nebbie::Exit exit = readExitItem(item);
    exit_info_->setText(QString("%1 -> stanza #%2")
                            .arg(QString::fromStdString(nebbie::exit_direction_label(exit.direction)))
                            .arg(exit.to_room));
    exit_description_->setText(QString::fromStdString(exit.description));
    updateLineLengthIndicators();
}
