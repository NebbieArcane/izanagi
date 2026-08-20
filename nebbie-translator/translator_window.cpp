#include "translator_window.hpp"

#include "app_config.hpp"
#include "translator_room_widget.hpp"

#include "path_util.hpp"

#include "nebbie/edit.hpp"
#include "nebbie/io.hpp"
#include "nebbie/validate.hpp"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

namespace {

void addListItem(QListWidget* list, long vnum, const QString& label) {
    list->addItem(label);
    list->item(list->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(vnum));
}

} // namespace

TranslatorWindow::TranslatorWindow(QWidget* parent) : QMainWindow(parent) {
    app_config_ = nebbie::translate::read_config();
    setupUi();
    setupMenus();
    room_editor_->setMaxLineLength(app_config_.max_line_length);
    setWindowTitle("Nebbie Translate");
    resize(960, 680);
    setStatus("Apri una libreria (mudroot/lib) per tradurre le descrizioni delle stanze.");
}

void TranslatorWindow::setupUi() {
    auto* central = new QWidget;
    auto* root_layout = new QVBoxLayout(central);

    lib_label_ = new QLabel("Nessuna libreria aperta");
    lib_label_->setWordWrap(true);
    root_layout->addWidget(lib_label_);

    auto* top = new QHBoxLayout;
    room_search_ = new QLineEdit;
    room_search_->setPlaceholderText("Cerca vnum o nome stanza...");
    top->addWidget(room_search_, 1);
    root_layout->addLayout(top);

    auto* splitter = new QSplitter;
    room_list_ = new QListWidget;
    room_list_->setMinimumWidth(200);
    splitter->addWidget(room_list_);

    auto* editor_panel = new QWidget;
    auto* editor_layout = new QVBoxLayout(editor_panel);
    editor_layout->setContentsMargins(0, 0, 0, 0);
    auto* room_scroll = new QScrollArea;
    room_scroll->setWidgetResizable(true);
    room_editor_ = new TranslatorRoomWidget;
    room_scroll->setWidget(room_editor_);
    editor_layout->addWidget(room_scroll, 1);

    auto* buttons = new QHBoxLayout;
    auto* apply_button = new QPushButton("Applica modifiche");
    buttons->addWidget(apply_button);
    buttons->addStretch();
    editor_layout->addLayout(buttons);
    splitter->addWidget(editor_panel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);
    splitter->setSizes({240, 720});
    root_layout->addWidget(splitter, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Pronto.");

    autosave_timer_ = new QTimer(this);
    autosave_timer_->setInterval(session_config_.autosave_interval_sec * 1000);
    connect(autosave_timer_, &QTimer::timeout, this, &TranslatorWindow::onAutosaveTick);
    connect(room_list_, &QListWidget::currentRowChanged, this, &TranslatorWindow::onRoomSelected);
    connect(room_search_, &QLineEdit::textChanged, this, &TranslatorWindow::onRoomSearchChanged);
    connect(apply_button, &QPushButton::clicked, this, &TranslatorWindow::applyRoomChanges);
}

void TranslatorWindow::setupMenus() {
    auto* file_menu = menuBar()->addMenu("&File");

    auto* open_action = file_menu->addAction("Apri libreria...");
    open_action->setShortcut(QKeySequence::Open);
    connect(open_action, &QAction::triggered, this, &TranslatorWindow::openLib);

    auto* save_action = file_menu->addAction("Salva");
    save_action->setShortcut(QKeySequence::Save);
    connect(save_action, &QAction::triggered, this, &TranslatorWindow::saveLib);

    auto* save_force_action = file_menu->addAction("Salva (forza)");
    connect(save_force_action, &QAction::triggered, this, &TranslatorWindow::saveLibForce);

    file_menu->addSeparator();
    file_menu->addAction("E&sci", this, &QWidget::close);

    auto* tools_menu = menuBar()->addMenu("&Strumenti");
    auto* validate_action = tools_menu->addAction("&Valida");
    validate_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(validate_action, &QAction::triggered, this, &TranslatorWindow::validateLib);

    auto* prefs_menu = menuBar()->addMenu("&Preferenze");
    auto* line_limit_action = prefs_menu->addAction("Limite caratteri per riga...");
    connect(line_limit_action, &QAction::triggered, this, &TranslatorWindow::editLineLengthLimit);
}

void TranslatorWindow::openLibPath(const QString& path) {
    try {
        const std::filesystem::path requested = nebbie::qt::path_from_qstring(path);
        const std::filesystem::path resolved = nebbie::resolve_lib_directory(requested);
        if (!std::filesystem::exists(resolved)) {
            QMessageBox::warning(this, "Apri libreria",
                                 QString("Percorso non valido:\n%1").arg(path));
            return;
        }
        if (requested != resolved) {
            setStatus(QString("Libreria risolta in: %1")
                          .arg(nebbie::qt::qstring_from_path(resolved)));
        }
        loadLib(resolved);
        rememberLibPath(resolved);
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Apri libreria", QString::fromUtf8(ex.what()));
    }
}

void TranslatorWindow::openLib() {
    if (!confirmSaveIfDirty()) {
        return;
    }

    const QString dir = QFileDialog::getExistingDirectory(
        this, "Apri libreria Nebbie (mudroot o mudroot/lib)", nebbie::translate::read_lib_path(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        return;
    }
    openLibPath(dir);
}

void TranslatorWindow::openStartupLib() {
    const QString saved = nebbie::translate::read_lib_path();
    if (nebbie::translate::lib_path_exists(saved)) {
        openLibPath(saved);
        return;
    }

    if (!saved.isEmpty()) {
        promptForLibPath(QString("Il percorso salvato non è più valido:\n%1").arg(saved));
        return;
    }

    promptForLibPath(QString(
        "Benvenuto in Nebbie Translate.\n\n"
        "Seleziona la cartella della libreria di gioco (mudroot o mudroot/lib).\n"
        "Il percorso verrà salvato in:\n%1")
                         .arg(nebbie::translate::default_config_path()));
}

bool TranslatorWindow::promptForLibPath(const QString& reason) {
    if (!reason.isEmpty()) {
        QMessageBox::information(this, "Nebbie Translate", reason);
    }

    const QString initial = nebbie::translate::read_lib_path();
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        "Seleziona libreria Nebbie (mudroot o mudroot/lib)",
        initial.isEmpty() ? QDir::homePath() : initial,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        setStatus("Nessuna libreria selezionata. Usa File → Apri libreria.");
        return false;
    }

    openLibPath(dir);
    return !lib_path_.empty();
}

void TranslatorWindow::loadLib(const std::filesystem::path& path) {
    world_.clear();
    context_ = {};
    nebbie::load_lib(world_, path, context_);
    lib_path_ = path;
    room_filter_.clear();
    room_search_->clear();
    markClean();
    refreshRoomList();

    lib_label_->setText(QString("Libreria: %1 — %2 zone, %3 stanze")
                            .arg(nebbie::qt::qstring_from_path(path))
                            .arg(world_.zones.size())
                            .arg(world_.rooms.size()));
    last_version_time_ = std::chrono::system_clock::now();
    autosave_timer_->start();
    setStatus(QString("Libreria caricata: %1 stanze.").arg(world_.rooms.size()));
}

void TranslatorWindow::refreshRoomList() {
    const long selected = currentRoomVnum();
    room_list_->clear();
    const std::string query = room_filter_.toStdString();
    for (const auto& [vnum, room] : world_.rooms) {
        if (!nebbie::entity_matches(vnum, room.name, query)) {
            continue;
        }
        addListItem(room_list_, vnum,
                    QString("#%1 %2").arg(vnum).arg(QString::fromStdString(room.name)));
    }
    if (selected > 0) {
        selectRoomByVnum(selected);
    } else if (room_list_->count() > 0 && room_list_->currentRow() < 0) {
        room_list_->setCurrentRow(0);
    }
}

void TranslatorWindow::selectRoomByVnum(const long vnum) {
    for (int i = 0; i < room_list_->count(); ++i) {
        if (room_list_->item(i)->data(Qt::UserRole).toLongLong() == vnum) {
            room_list_->setCurrentRow(i);
            return;
        }
    }
}

long TranslatorWindow::currentRoomVnum() const {
    const auto* item = room_list_->currentItem();
    if (!item) {
        return -1;
    }
    return static_cast<long>(item->data(Qt::UserRole).toLongLong());
}

void TranslatorWindow::onRoomSelected() {
    const long vnum = currentRoomVnum();
    if (vnum <= 0) {
        return;
    }
    const nebbie::Room* room = world_.find_room(vnum);
    if (!room) {
        return;
    }
    room_editor_->loadFromRoom(*room);
}

void TranslatorWindow::onRoomSearchChanged(const QString& text) {
    room_filter_ = text;
    refreshRoomList();
}

void TranslatorWindow::applyRoomChanges() {
    const long vnum = currentRoomVnum();
    if (vnum <= 0) {
        QMessageBox::information(this, "Stanze", "Seleziona una stanza.");
        return;
    }

    nebbie::Room* room = world_.find_room(vnum);
    if (!room) {
        QMessageBox::warning(this, "Stanze", "Stanza non trovata.");
        return;
    }

    const std::string old_name = room->name;
    nebbie::Room updated = *room;
    room_editor_->saveTranslatableFields(*room, updated);
    nebbie::assign_room_fields(*room, updated);

    std::size_t aligned = 0;
    if (old_name != room->name) {
        aligned = nebbie::refresh_inbound_exit_descriptions(
            world_, vnum, nebbie::InboundExitAlignPolicy::SyncDestinationName, &old_name);
    }

    if (auto* item = room_list_->currentItem()) {
        item->setText(QString("#%1 %2").arg(vnum).arg(QString::fromStdString(room->name)));
    }

    markDirty();
    if (aligned > 0) {
        setStatus(QString("Stanza %1 aggiornata; %2 description collegate sincronizzate.")
                      .arg(vnum)
                      .arg(static_cast<qlonglong>(aligned)));
    } else {
        setStatus(QString("Stanza %1 aggiornata in memoria.").arg(vnum));
    }
}

void TranslatorWindow::validateLib() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Valida", "Apri prima una libreria.");
        return;
    }

    const nebbie::ValidationReport report = nebbie::validate_world(world_, validationOptions());
    showValidation(report);
    if (report.ok()) {
        if (report.warning_count() > 0) {
            setStatus(QString("Validazione OK con %1 avvisi.").arg(report.warning_count()));
        } else {
            setStatus("Validazione OK.");
        }
    } else {
        setStatus(QString("Validazione: %1 errori.").arg(report.error_count()));
    }
}

void TranslatorWindow::showValidation(const nebbie::ValidationReport& report) {
    QString message;
    if (report.ok() && report.warning_count() == 0) {
        message = "Nessun errore di validazione.";
    } else {
        message = QString("Errori: %1\nAvvisi: %2\n\n").arg(report.error_count()).arg(report.warning_count());
        for (const auto& issue : report.issues) {
            const char* level = issue.severity == nebbie::ValidationSeverity::error ? "ERR" : "WARN";
            message += QString("[%1] %2\n").arg(level).arg(QString::fromStdString(issue.message));
            if (message.size() > 12000) {
                message += "…\n";
                break;
            }
        }
    }
    QMessageBox::information(this, "Validazione", message);
}

void TranslatorWindow::saveLib() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Salva", "Apri prima una libreria.");
        return;
    }

    const nebbie::ValidationReport report = nebbie::validate_world(world_, validationOptions());
    if (!report.ok()) {
        showValidation(report);
        const auto answer = QMessageBox::question(
            this, "Errori di validazione",
            QString("Ci sono %1 errori. Salvare comunque?").arg(report.error_count()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    try {
        nebbie::save_lib_with_backup(world_, context_, lib_path_);
        markClean();
        last_version_time_ = std::chrono::system_clock::now();
        setStatus("Libreria salvata (backup in .nebbie/versions).");
        QMessageBox::information(this, "Salva", "Salvataggio completato.");
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
    }
}

void TranslatorWindow::saveLibForce() {
    if (lib_path_.empty()) {
        QMessageBox::information(this, "Salva", "Apri prima una libreria.");
        return;
    }
    try {
        nebbie::save_lib_with_backup(world_, context_, lib_path_);
        markClean();
        last_version_time_ = std::chrono::system_clock::now();
        setStatus("Libreria salvata (forzato).");
    } catch (const std::exception& ex) {
        QMessageBox::critical(this, "Errore", QString::fromUtf8(ex.what()));
    }
}

void TranslatorWindow::onAutosaveTick() {
    if (!dirty_ || lib_path_.empty()) {
        return;
    }

    try {
        const auto result = nebbie::run_autosave(world_, context_, lib_path_, session_config_, last_version_time_);
        if (result.version_created) {
            last_version_time_ = std::chrono::system_clock::now();
        }
        const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
        if (result.version_created) {
            setStatus(QString("Autosalvataggio + versione %1 (%2)")
                          .arg(QString::fromStdString(result.version_id))
                          .arg(time));
        } else {
            setStatus(QString("Autosalvataggio workspace (%1)").arg(time));
        }
    } catch (const std::exception& ex) {
        setStatus(QString("Autosalvataggio fallito: %1").arg(QString::fromUtf8(ex.what())));
    }
}

void TranslatorWindow::rememberLibPath(const std::filesystem::path& path) {
    app_config_.lib_path = nebbie::qt::qstring_from_path(path);
    nebbie::translate::write_config(app_config_);
}

nebbie::ValidationOptions TranslatorWindow::validationOptions() const {
    nebbie::ValidationOptions options;
    options.max_line_length = app_config_.max_line_length;
    return options;
}

void TranslatorWindow::editLineLengthLimit() {
    bool ok = false;
    const int value = QInputDialog::getInt(
        this,
        "Limite caratteri per riga",
        "Numero massimo di caratteri per riga nei testi traducibili.\n"
        "Imposta 0 per disattivare il controllo.",
        app_config_.max_line_length,
        0,
        512,
        1,
        &ok);
    if (!ok) {
        return;
    }

    app_config_.max_line_length = value;
    nebbie::translate::write_config(app_config_);
    room_editor_->setMaxLineLength(app_config_.max_line_length);
    if (app_config_.max_line_length > 0) {
        setStatus(QString("Limite righe impostato a %1 caratteri.").arg(app_config_.max_line_length));
    } else {
        setStatus("Controllo lunghezza righe disattivato.");
    }
}

void TranslatorWindow::markDirty() {
    dirty_ = true;
    const QString title = windowTitle();
    if (!title.startsWith('*')) {
        setWindowTitle("* " + title);
    }
}

void TranslatorWindow::markClean() {
    dirty_ = false;
    QString title = windowTitle();
    while (title.startsWith("* ")) {
        title.remove(0, 2);
    }
    setWindowTitle(title.isEmpty() ? QStringLiteral("Nebbie Translate") : title);
}

bool TranslatorWindow::confirmSaveIfDirty() {
    if (!dirty_) {
        return true;
    }

    const auto answer = QMessageBox::question(
        this, "Modifiche non salvate",
        "Ci sono modifiche non salvate. Salvare prima di continuare?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Save) {
        saveLib();
        return !dirty_;
    }
    markClean();
    return true;
}

void TranslatorWindow::setStatus(const QString& text) {
    statusBar()->showMessage(text);
}

void TranslatorWindow::closeEvent(QCloseEvent* event) {
    if (!confirmSaveIfDirty()) {
        event->ignore();
        return;
    }
    event->accept();
}
