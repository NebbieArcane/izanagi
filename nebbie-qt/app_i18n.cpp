#include "app_i18n.hpp"
#include "version.hpp"

#include <QHash>
#include <QStringList>

namespace nebbie::qt {

namespace {

AppLanguage g_language = AppLanguage::Italian;

struct TranslationEntry {
    const char* key;
    const char* italian;
    const char* english;
};

constexpr TranslationEntry kTranslations[] = {
    {"menu.file", "&File", "&File"},
    {"menu.open_lib", "Apri lib &monolite (myst.* / mudroot/lib)...",
     "Open &monolith lib (myst.* / mudroot/lib)..."},
    {"menu.open_lib_tip",
     "Apre una libreria Nebbie classica: file myst.zon, myst.wld, myst.mob, myst.obj nella stessa cartella "
     "(mudroot o mudroot/lib). Non usare per le cartelle area del repo Aree.",
     "Opens a classic Nebbie library: myst.zon, myst.wld, myst.mob, myst.obj in one folder "
     "(mudroot or mudroot/lib). Do not use for Aree repo area folders."},
    {"menu.open_sample_lib", "Apri mondo di e&sempio...", "Open &sample world..."},
    {"menu.open_sample_lib_tip",
     "Apre la lib di prova inclusa nel pacchetto (sample-mudroot). Utile per provare l'editor senza il tuo mudroot.",
     "Opens the bundled sample lib (sample-mudroot). Useful to try the editor without your mudroot."},
    {"dialog.sample_lib_missing",
     "Mondo di esempio non trovato in questo install.\n"
     "Nei pacchetti ufficiali e' dentro l'app (macOS) o in /usr/share/nebbie-editor/sample-mudroot (Linux).",
     "Sample world not found in this install.\n"
     "Official packages ship it inside the app (macOS) or under /usr/share/nebbie-editor/sample-mudroot (Linux)."},
    {"menu.open_aree_workspace", "Apri workspace &Aree (cartelle area)...",
     "Open Aree &workspace (area folders)..."},
    {"menu.open_aree_workspace_tip",
     "Apre la root del repo Aree (o altra cartella con sottocartelle castelli/, myst/, …). "
     "Scegli un'area dalla lista per modificarla senza riscrivere tutto il monolite.",
     "Opens the Aree repo root (or another folder with castelli/, myst/, … subfolders). "
     "Pick an area from the list to edit in place without rewriting the whole monolith."},
    {"menu.aree_open_area", "Apri area selezionata", "Open selected area"},
    {"menu.aree_restore_archive", "Ripristina area da archivio...", "Restore area from archive..."},
    {"menu.reload_lib", "A&ggiorna libreria", "R&eload library"},
    {"menu.reload_lib_tip",
     "Ricarica dal disco la cartella libreria attualmente aperta.",
     "Reload the currently open library folder from disk."},
    {"menu.save", "&Salva", "&Save"},
    {"menu.save_force", "Salva (forza)", "Save (force)"},
    {"menu.history", "Cronologia", "History"},
    {"menu.restore_autosave", "Ripristina ultimo autosalvataggio", "Restore last autosave"},
    {"menu.restore_version", "Ripristina versione...", "Restore version..."},
    {"menu.exit", "E&sci", "E&xit"},
    {"menu.edit", "&Modifica", "&Edit"},
    {"menu.undo", "&Annulla", "&Undo"},
    {"menu.redo", "&Ripeti", "&Redo"},
    {"menu.cut", "&Taglia", "Cu&t"},
    {"menu.copy", "&Copia", "&Copy"},
    {"menu.paste", "I&ncolla", "&Paste"},
    {"menu.paste_plain", "Incolla senza formattazione", "Paste plain text"},
    {"menu.select_all", "Seleziona &tutto", "Select &all"},
    {"menu.tools", "&Strumenti", "&Tools"},
    {"menu.validate_world", "Controlla l'area (o lib) aperta...",
     "Check open area (or lib)..."},
    {"menu.validate_world_tip",
     "Controlla errori e avvisi su ciò che hai aperto adesso: se stai lavorando "
     "su un'area del workspace, solo quell'area; se hai aperto la lib monolite, "
     "tutta la lib. Controlla stanze, mob, oggetti, zone/reset, shop e altro, "
     "e segnala anche possibili testi inglesi residui ancora da tradurre. "
     "Non controlla tutte le aree del workspace in un colpo solo.",
     "Checks errors and warnings in what you have open now: if you opened an area "
     "from the workspace, only that area; if you opened the monolith lib, the whole "
     "lib. Checks rooms, mobs, objects, zones/resets, shops, and more, and also "
     "flags possible leftover English text still to translate. "
     "Does not check every area in the workspace at once."},
    {"menu.validate", "&Valida", "&Validate"},
    {"menu.help_guide", "&Guida ai menu...", "&Menu guide..."},
    {"menu.export_overlays", "Esporta overlay...", "Export overlays..."},
    {"menu.align_exits", "Normalizza description uscite (mondo)...",
     "Normalize exit descriptions (world)..."},
    {"menu.align_exits_tip",
     "Normalizza description che differiscono dal name di destinazione solo per spazi/newline. "
     "Preserva description vuote, formato [vnum (nome)] e testi di look personalizzati.",
     "Normalize descriptions that differ from the destination name only by trailing "
     "spaces/newlines. Preserves empty descriptions, [vnum (name)] format, and custom look text."},
    {"menu.fix_mud_ascii_field", "Correggi non-ASCII nel campo...",
     "Fix non-ASCII in field..."},
    {"menu.fix_mud_ascii_field_tip",
     "Nel campo di testo attivo: accenti → a'/e'/… e apostrofi tipografici → apice ASCII '.",
     "In the focused text field: accents → a'/e'/… and curly apostrophes → ASCII '."},
    {"menu.fix_mud_ascii_room", "Correggi non-ASCII nella stanza...",
     "Fix non-ASCII in room..."},
    {"menu.fix_mud_ascii_room_tip",
     "Corregge nome, descrizione, extra, uscite e campi luce della stanza aperta.",
     "Fixes name, description, extras, exits and light fields of the open room."},
    {"status.mud_ascii_fixed", "Testo corretto in ASCII mud (accenti/apostrofi).",
     "Text normalized to mud ASCII (accents/apostrophes)."},
    {"status.mud_ascii_unchanged", "Nessun carattere non-ASCII da correggere.",
     "No non-ASCII characters to fix."},
    {"status.mud_ascii_need_field", "Metti il focus su un campo di testo mud.",
     "Focus a mud text field first."},
    {"menu.prefs", "&Preferenze", "&Preferences"},
    {"menu.line_limit", "Limite caratteri per riga...", "Line length limit..."},
    {"menu.extended_colors", "Visualizzazione estesa codici colore", "Extended color code view"},
    {"menu.language", "Lingua / Language", "Lingua / Language"},
    {"menu.language_it", "Italiano", "Italiano"},
    {"menu.language_en", "English", "English"},
    {"menu.colors", "&Colori", "&Colors"},
    {"menu.color_legend", "Legenda colori MUD", "MUD color legend"},
    {"menu.insert_color", "Inserisci codice...", "Insert code..."},
    {"menu.coordinator", "&Coordinator", "&Coordinator"},
    {"menu.coordinator_config", "Configuration...", "Configuration..."},
    {"menu.refresh_index", "Refresh world index", "Refresh world index"},
    {"menu.load_index", "Load world index from file...", "Load world index from file..."},
    {"menu.export_index", "Export local world index", "Export local world index"},
    {"menu.reserve_vnums", "Reserve vnums...", "Reserve vnums..."},
    {"menu.help", "&Aiuto", "&Help"},
    {"menu.check_updates", "Controlla aggiornamenti...", "Check for updates..."},
    {"menu.check_updates_startup", "Controlla aggiornamenti all'avvio",
     "Check for updates on startup"},
    {"menu.about_izanagi", "Informazioni su Izanagi...", "About Izanagi..."},
    {"menu.about_cypher", "Informazioni su Cypher...", "About Cypher..."},
    {"status.no_lib", "Nessuna libreria aperta", "No library open"},
    {"status.open_lib_to_start",
     "Apri un workspace Aree (File → Apri workspace Aree) o un'area dalla lista.",
     "Open an Aree workspace (File → Open Aree workspace) or pick an area from the list."},
    {"status.open_lib_translate",
     "Apri una libreria (mudroot/lib) per tradurre le descrizioni delle stanze.",
     "Open a library (mudroot/lib) to translate room descriptions."},
    {"status.ready", "Pronto.", "Ready."},
    {"status.extended_on",
     "Visualizzazione estesa attiva: i codici $cXXXX sono visibili.",
     "Extended view enabled: $cXXXX codes are visible."},
    {"status.extended_off", "Codici colore nascosti: anteprima colorata attiva.",
     "Color codes hidden: colored preview active."},
    {"ui.search_vnum", "Cerca vnum o nome...", "Search vnum or name..."},
    {"ui.search_room", "Cerca vnum o nome stanza...", "Search vnum or room name..."},
    {"ui.new_room", "Nuova stanza", "New room"},
    {"ui.new_mob", "Nuovo mob", "New mob"},
    {"ui.new_object", "Nuovo oggetto", "New object"},
    {"ui.new_zone", "Nuova zona", "New zone"},
    {"ui.apply_changes", "Applica modifiche", "Apply changes"},
    {"ui.apply_changes_en", "Apply changes", "Apply changes"},
    {"ui.sync_exit_desc", "Sincronizza description verso questa stanza",
     "Sync descriptions toward this room"},
    {"ui.sync_exit_desc_tip",
     "Per le uscite verso la stanza selezionata: aggiorna solo le description che "
     "corrispondevano al vecchio name (rinomina) o che differiscono solo per spazi/newline. "
     "Le description vuote, legacy [vnum (nome)] e i testi di look personalizzati restano invariati.",
     "For exits toward the selected room: update only descriptions that matched the old "
     "name (rename) or differ only by spaces/newlines. Empty descriptions, legacy "
     "[vnum (name)] format, and custom look text stay unchanged."},
    {"ui.align_exits_world", "Normalizza description uscite (mondo)",
     "Normalize exit descriptions (world)"},
    {"ui.align_exits_world_tip",
     "Normalizza nel mondo le description che differiscono dal name di destinazione solo "
     "per spazi o newline finali. Non modifica description vuote, formato [vnum (nome)] "
     "né testi di look personalizzati (Vedi..., descrizioni lunghe, ecc.).",
     "Normalize world-wide descriptions that differ from the destination name only by "
     "trailing spaces or newlines. Does not change empty descriptions, [vnum (name)] "
     "format, or custom look text."},
    {"ui.goto_exit", "Vai alla stanza di uscita", "Go to exit target"},
    {"tab.rooms", "Stanze", "Rooms"},
    {"tab.mob", "Mob", "Mob"},
    {"tab.objects", "Oggetti", "Objects"},
    {"tab.zone", "Zone", "Zones"},
    {"tab.shops", "Negozi", "Shops"},
    {"tab.specials", "Special", "Specials"},
    {"tab.global_tables", "Tabelle globali", "Global tables"},
    {"tab.world_data", "Dati mondo", "World data"},
    {"tab.map", "Mappa", "Map"},
    {"tab.validation", "Validazione", "Validation"},
    {"tab.map_zone", "Zona", "Zone"},
    {"tab.map_world", "Mondo (zone)", "World (zones)"},
    {"aree.dock_title", "Workspace Aree", "Aree workspace"},
    {"aree.col_folder", "Cartella", "Folder"},
    {"aree.col_zone", "N. area", "Area #"},
    {"aree.col_name", "Nome", "Name"},
    {"aree.col_range", "Stanze", "Rooms"},
    {"zone.col_num", "N. area", "Area #"},
    {"zone.col_name", "Nome", "Name"},
    {"zone.col_rooms", "Stanze", "Rooms"},
    {"aree.open_area", "Apri area", "Open area"},
    {"aree.archive_area", "Archivia area", "Archive area"},
    {"aree.restore_archive", "Ripristina da archivio", "Restore from archive"},
    {"aree.no_workspace", "Nessun workspace Aree aperto.", "No Aree workspace open."},
    {"aree.select_area", "Seleziona un'area dalla lista.", "Select an area from the list."},
    {"aree.no_open_area", "Nessuna area aperta da archiviare.", "No open area to archive."},
    {"aree.archive_title", "Archivia area", "Archive area"},
    {"aree.archive_prompt", "Commento archivio per \"%1\" (opzionale):",
     "Archive comment for \"%1\" (optional):"},
    {"aree.archive_comment", "Commento archivio (opzionale)", "Archive comment (optional)"},
    {"aree.archive_failed", "Archivio area fallito:\n%1", "Area archive failed:\n%1"},
    {"aree.archive_ok", "Area \"%1\" archiviata.", "Area \"%1\" archived."},
    {"aree.archive_on_first_save", "Archivia area al primo Salva della sessione",
     "Archive area on first Save of the session"},
    {"aree.goto_other_area",
     "La stanza #%1 e' nell'area \"%2\". Aprirla e andare alla stanza?",
     "Room #%1 is in area \"%2\". Open it and go to that room?"},
    {"aree.goto_missing_room",
     "Stanza #%1 non trovata nell'area aperta ne' nel workspace Aree.",
     "Room #%1 was not found in the open area or the Aree workspace."},
    {"aree.restore_title", "Ripristina da archivio", "Restore from archive"},
    {"aree.restore_prompt", "Scegli un archivio per l'area \"%1\":",
     "Choose an archive for area \"%1\":"},
    {"aree.no_archives", "Nessun archivio trovato per questa area.",
     "No archives found for this area."},
    {"aree.restore_confirm",
     "Ripristinare l'area \"%1\" dall'archivio \"%2\"?\n"
     "La cartella area corrente verrà sovrascritta.",
     "Restore area \"%1\" from archive \"%2\"?\n"
     "The current area folder will be overwritten."},
    {"aree.restore_ok", "Area ripristinata da archivio.", "Area restored from archive."},
    {"aree.restore_failed", "Ripristino fallito:\n%1", "Restore failed:\n%1"},
    {"aree.status_loaded", "Area Aree: %1 — %2 zone, %3 stanze",
     "Aree area: %1 — %2 zones, %3 rooms"},
    {"aree.status_workspace", "Workspace Aree: %1 (%2 aree)",
     "Aree workspace: %1 (%2 areas)"},
    {"aree.lib_open_block_title", "Percorso Aree — usa workspace Aree",
     "Aree path — use Aree workspace"},
    {"aree.lib_open_block_area",
     "La cartella selezionata è un'area Aree (es. myst/myst.zon), non una lib monolite.\n\n"
     "Aprirla con «Apri lib monolite» riscriverebbe i file al salvataggio (marker EOF, formato bit).\n\n"
     "Percorso: %1\n"
     "Workspace consigliato: %2",
     "The selected folder is an Aree area (e.g. myst/myst.zon), not a monolith library.\n\n"
     "Opening it via «Open monolith lib» would rewrite files on save (EOF markers, bit format).\n\n"
     "Path: %1\n"
     "Suggested workspace: %2"},
    {"aree.lib_open_block_workspace",
     "La cartella selezionata è un workspace Aree (contiene cartelle area), non myst.* in un'unica cartella.\n\n"
     "Usa «Apri workspace Aree» e poi seleziona l'area da modificare.\n\n"
     "Percorso: %1",
     "The selected folder is an Aree workspace (it contains area folders), not myst.* in one folder.\n\n"
     "Use «Open Aree workspace» and then pick the area to edit.\n\n"
     "Path: %1"},
    {"aree.lib_open_use_workspace", "Apri workspace Aree", "Open Aree workspace"},
    {"dialog.open_lib_title", "Apri lib monolite (myst.* / mudroot/lib)",
     "Open monolith lib (myst.* / mudroot/lib)"},
    {"dialog.startup_title", "Benvenuto in Izanagi", "Welcome to Izanagi"},
    {"dialog.open_startup_aree_first",
     "Izanagi è pensato per lavorare sulle cartelle area del repo Aree "
     "(es. castelli/castelli.zon, castelli.wld, …).\n\n"
     "Scegli «Apri workspace Aree» per la root del repo (o altra cartella con sotto-cartelle area).\n"
     "La libreria monolite mudroot/lib (myst.*) resta disponibile solo se la scegli esplicitamente.\n\n"
     "Config: %1",
     "Izanagi is designed for Aree-style area folders "
     "(e.g. castelli/castelli.zon, castelli.wld, …).\n\n"
     "Choose «Open Aree workspace» for the repo root (or any folder with area subfolders).\n"
     "The mudroot/lib monolith (myst.*) is only opened when you choose it explicitly.\n\n"
     "Config: %1"},
    {"dialog.startup_open_aree", "Apri workspace Aree…", "Open Aree workspace…"},
    {"dialog.startup_open_monolith", "Apri lib monolite (mudroot/lib)…",
     "Open monolith lib (mudroot/lib)…"},
    {"status.open_lib_cancelled",
     "Nessuna libreria selezionata. Usa File → Apri lib monolite o Apri workspace Aree.",
     "No library selected. Use File → Open monolith lib or Open Aree workspace."},
    {"update.title_error", "Aggiornamenti %1", "Updates %1"},
    {"update.check_failed", "Impossibile verificare gli aggiornamenti:\n%1",
     "Unable to check for updates:\n%1"},
    {"update.available_title", "Aggiornamento disponibile", "Update available"},
    {"update.available_body",
     "%1 %2 è disponibile (versione attuale: %3).\n\n"
     "Scarica il pacchetto e installalo per restare aggiornato.",
     "%1 %2 is available (current version: %3).\n\n"
     "Download the package and install it to stay up to date."},
    {"update.download", "Scarica aggiornamento", "Download update"},
    {"update.later", "Più tardi", "Later"},
    {"update.ignore", "Ignora questa versione", "Ignore this version"},
    {"update.up_to_date", "Stai usando l'ultima versione (%1).",
     "You are using the latest version (%1)."},
    {"update.invalid_response", "Risposta release GitHub non valida",
     "Invalid GitHub release response"},
    {"update.no_package", "Nessun pacchetto %1 trovato per questa piattaforma",
     "No %1 package found for this platform"},
    {"branding.izanagi_tagline",
     "Izanagi — un editor di mondo MUD completamente nuovo per Nebbie Arcane",
     "Izanagi — a totally new Nebbie Arcane MUD world editor"},
    {"branding.cypher_tagline",
     "Cypher — editor leggero solo stanze per Nebbie Arcane",
     "Cypher — a lightweight room-only MUD editor for Nebbie Arcane"},
    {"branding.izanagi_about",
     "Versione %1\n\n%2",
     "Version %1\n\n%2"},
    {"branding.cypher_about",
     "Versione %1\n\n%2",
     "Version %1\n\n%2"},
};

const TranslationEntry* findEntry(const char* key) {
    for (const TranslationEntry& entry : kTranslations) {
        if (QString::fromUtf8(entry.key) == QString::fromUtf8(key)) {
            return &entry;
        }
    }
    return nullptr;
}

QString substituteArgs(QString text, const QStringList& args) {
    for (int index = 0; index < args.size(); ++index) {
        text.replace(QStringLiteral("%") + QString::number(index + 1), args.at(index));
    }
    return text;
}

} // namespace

AppLanguage currentAppLanguage() {
    return g_language;
}

void setAppLanguage(AppLanguage language) {
    g_language = language;
}

AppLanguage parseLanguageCode(const QString& code) {
    const QString normalized = code.trimmed().toLower();
    if (normalized == QStringLiteral("en") || normalized == QStringLiteral("english")) {
        return AppLanguage::English;
    }
    return AppLanguage::Italian;
}

QString languageCode(AppLanguage language) {
    switch (language) {
    case AppLanguage::English:
        return QStringLiteral("en");
    case AppLanguage::Italian:
    default:
        return QStringLiteral("it");
    }
}

QString appTr(const char* key) {
    const TranslationEntry* entry = findEntry(key);
    if (!entry) {
        return QString::fromUtf8(key);
    }
    return g_language == AppLanguage::English ? QString::fromUtf8(entry->english)
                                              : QString::fromUtf8(entry->italian);
}

QString appTr(const char* key, const QString& arg1) {
    return substituteArgs(appTr(key), {arg1});
}

QString appTr(const char* key, const QString& arg1, const QString& arg2) {
    return substituteArgs(appTr(key), {arg1, arg2});
}

QString appTr(const char* key, const QString& arg1, const QString& arg2, const QString& arg3) {
    return substituteArgs(appTr(key), {arg1, arg2, arg3});
}

QString izanagiDisplayName() {
    return QStringLiteral("Izanagi");
}

QString cypherDisplayName() {
    return QStringLiteral("Cypher");
}

QString izanagiWindowTitle() {
    return QStringLiteral("%1 — %2").arg(izanagiDisplayName(), appTr("branding.izanagi_tagline"));
}

QString cypherWindowTitle() {
    return QStringLiteral("%1 — %2").arg(cypherDisplayName(), appTr("branding.cypher_tagline"));
}

QString izanagiAboutText(const QString& version) {
    return appTr("branding.izanagi_about", version, appTr("branding.izanagi_tagline"));
}

QString cypherAboutText(const QString& version) {
    return appTr("branding.cypher_about", version, appTr("branding.cypher_tagline"));
}

QString githubReleaseRepo() {
    return QStringLiteral("NebbieArcane/izanagi");
}

QString applicationVersionLabel() {
    QString version = QStringLiteral(NEBBIE_VERSION);
    if (QStringLiteral(NEBBIE_GIT_SHA) != QStringLiteral("dev")) {
        version += QStringLiteral(" (") + QStringLiteral(NEBBIE_GIT_SHA) + QLatin1Char(')');
    }
    return version;
}

} // namespace nebbie::qt
