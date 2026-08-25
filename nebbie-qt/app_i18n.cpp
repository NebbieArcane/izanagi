#include "app_i18n.hpp"

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
    {"menu.open_lib", "&Apri libreria...", "&Open library..."},
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
    {"menu.tools", "&Strumenti", "&Tools"},
    {"menu.validate_world", "Valida &mondo intero...", "Validate &entire world..."},
    {"menu.validate_world_tip",
     "Validazione completa: stanze, reset, shop, guild, special, social.",
     "Full validation: rooms, resets, shops, guilds, specials, socials."},
    {"menu.validate", "&Valida", "&Validate"},
    {"menu.export_overlays", "Esporta overlay...", "Export overlays..."},
    {"menu.split_zones", "Dividi per zone...", "Split by zones..."},
    {"menu.split_zones_tip",
     "Esporta ogni zona in una sottodirectory con myst.zon/wld/mob/obj filtrati "
     "e directory overlay rooms/objects/mobiles/zones.",
     "Export each zone into a subdirectory with filtered myst.zon/wld/mob/obj "
     "and overlay rooms/objects/mobiles/zones directories."},
    {"menu.align_exits", "Normalizza description uscite (mondo)...",
     "Normalize exit descriptions (world)..."},
    {"menu.align_exits_tip",
     "Normalizza description che differiscono dal name di destinazione solo per spazi/newline. "
     "Preserva description vuote, formato [vnum (nome)] e testi di look personalizzati.",
     "Normalize descriptions that differ from the destination name only by trailing "
     "spaces/newlines. Preserves empty descriptions, [vnum (name)] format, and custom look text."},
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
    {"status.open_lib_to_start", "Apri una libreria (mudroot/lib) per iniziare.",
     "Open a library (mudroot/lib) to get started."},
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
    {"tab.world_data", "Dati mondo", "World data"},
    {"tab.map", "Mappa", "Map"},
    {"tab.validation", "Validazione", "Validation"},
    {"tab.map_zone", "Zona", "Zone"},
    {"tab.map_world", "Mondo (zone)", "World (zones)"},
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
     "Izanagi %1\n\n%2",
     "Izanagi %1\n\n%2"},
    {"branding.cypher_about",
     "Cypher %1\n\n%2",
     "Cypher %1\n\n%2"},
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
    return QStringLiteral("wizardmorgan/nebbie-arcane-editing-tools");
}

} // namespace nebbie::qt
