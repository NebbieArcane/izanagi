#include "help_menu_guide.hpp"

#include "app_i18n.hpp"

#include <QDialog>
#include <QDialogButtonBox>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace nebbie::qt {
namespace {

QString menuGuideHtmlItalian() {
    return QStringLiteral(R"HTML(
<h2>Guida ai menu di Izanagi</h2>
<p>Spiegazione semplice di ogni voce. Il modo consigliato di lavorare è il
<strong>workspace Aree</strong> (cartelle area), non la lib monolite.</p>

<h3>File</h3>
<ul>
<li><b>Apri workspace Aree</b> — Scegli la cartella principale del mondo a pezzi
(es. quella che contiene sottocartelle come <code>castelli/</code>, <code>myst/</code>).
Poi dalla lista a sinistra apri l’area che vuoi modificare.</li>
<li><b>Apri lib monolite</b> — Solo se lavori ancora sulla lib unica del server
(<code>mudroot/lib</code> con i file <code>myst.*</code>). Non usarlo per le cartelle area.</li>
<li><b>Aggiorna libreria</b> — Ricarica da disco ciò che hai aperto, scartando le
modifiche non salvate in memoria.</li>
<li><b>Salva</b> — Scrive le modifiche sull’area (o sulla lib) aperta. Prima crea
una copia di sicurezza.</li>
<li><b>Salva (forza)</b> — Salva anche se ci sono avvisi che di solito fermano il salvataggio.</li>
<li><b>Cronologia → Ripristina ultimo autosalvataggio</b> — Torna all’ultima
copia automatica fatta mentre lavoravi.</li>
<li><b>Cronologia → Ripristina versione…</b> — Scegli una copia precedente
(salvata automaticamente o prima di un Salva).</li>
<li><b>Esci</b> — Chiude Izanagi.</li>
</ul>
<p>Nel pannello <b>Workspace Aree</b> (a sinistra, dopo aver aperto il workspace):</p>
<ul>
<li><b>Apri area</b> — Carica subito l’area selezionata (senza dialogo).</li>
<li><b>Archivia area</b> — Crea una copia di sicurezza dell’area aperta
(o di quella selezionata). Opzionale: Preferenze → archivia al primo Salva.</li>
<li><b>Ripristina da archivio</b> — Riporta l’area a una copia di sicurezza
fatta in precedenza (vedi path sotto).</li>
</ul>

<h3>Modifica</h3>
<ul>
<li><b>Annulla / Ripeti</b> — Indietro e avanti sulle modifiche di testo.</li>
<li><b>Taglia / Copia / Incolla</b> — Come in qualsiasi editor.</li>
<li><b>Incolla senza formattazione</b> — Incolla solo testo semplice, senza stili.</li>
<li><b>Seleziona tutto</b> — Seleziona tutto il testo del campo attivo.</li>
</ul>

<h3>Strumenti</h3>
<ul>
<li><b>Controlla l’area (o lib) aperta…</b> — Cerca errori e avvisi su
<strong>ciò che hai aperto adesso</strong>:
<ul>
<li>se hai aperto un’area dal workspace → controlla <em>solo quell’area</em>
(stanze, mob, oggetti, zone, negozi, ecc.);</li>
<li>se hai aperto la lib monolite → controlla tutta la lib caricata.</li>
</ul>
Non controlla tutte le aree del workspace in un colpo solo. Al salvataggio
Izanagi fa un controllo più ristretto (soprattutto sulle stanze modificate);
questa voce serve per un controllo completo di ciò che è aperto.</li>
<li><b>Esporta overlay…</b> — Crea i file “overlay” usati dal server a partire
da ciò che è aperto (uso avanzato / deploy).</li>
<li><b>Normalizza description uscite…</b> — Sistema le descrizioni delle uscite
che differiscono dal nome della stanza di destinazione solo per spazi o a capo
inutili. Non cancella descrizioni personalizzate.</li>
</ul>

<h3>Preferenze</h3>
<ul>
<li><b>Limite caratteri per riga…</b> — Quanti caratteri per riga nei testi
(utile per stare nei limiti del MUD).</li>
<li><b>Visualizzazione estesa codici colore</b> — Mostra o nasconde i codici
colore grezzi (tipo <code>$cXXXX</code>) nei campi testo.</li>
<li><b>Lingua / Language</b> — Italiano o English per i menu.</li>
</ul>

<h3>Colori</h3>
<ul>
<li><b>Legenda colori MUD</b> — Elenco dei colori disponibili nel gioco.</li>
<li><b>Inserisci codice…</b> — Inserisce un codice colore nel testo dove stai scrivendo.</li>
</ul>

<h3>Coordinator</h3>
<p>Strumenti per chi coordina i numeri delle zone (vnum) tra più persone.
Nella maggior parte del lavoro quotidiano sulle aree puoi ignorarli.</p>
<ul>
<li><b>Configuration…</b> — Indirizzo e impostazioni del servizio Coordinator.</li>
<li><b>Refresh / Load / Export world index</b> — Aggiorna, carica o esporta l’elenco
delle zone e dei range di numeri usati nel mondo.</li>
<li><b>Reserve vnums…</b> — Prenota un intervallo di numeri per una nuova zona,
così non si sovrappone ad altre.</li>
</ul>

<h3>Aiuto</h3>
<ul>
<li><b>Controlla aggiornamenti…</b> — Verifica se c’è una versione nuova di Izanagi.</li>
<li><b>Controlla aggiornamenti all’avvio</b> — Se attivo, lo fa automaticamente aprendo l’app.</li>
<li><b>Guida ai menu…</b> — Questa pagina.</li>
<li><b>Informazioni su Izanagi…</b> — Versione e crediti.</li>
</ul>

<h2>Dove sono i salvataggi di sicurezza</h2>
<p>Izanagi non li mette “da qualche parte sul PC a caso”: stanno vicino al
workspace (o alla lib) che hai aperto. Sostituisci i pezzi tra parentesi angolari
con i tuoi percorsi reali.</p>

<h3>Uso normale — workspace Aree</h3>
<p>Esempio: workspace in <code>~/NebbieArcane/Aree</code>, area <code>myst</code>.</p>
<ul>
<li><b>Archivi dell’area</b> (copia intera della cartella, es. all’apertura):<br>
<code>&lt;cartella-workspace&gt;/.izanagi/archives/&lt;nome-area&gt;/&lt;data-ora&gt;/</code><br>
Esempio: <code>~/NebbieArcane/Aree/.izanagi/archives/myst/2026-04-10_123045/</code></li>
<li><b>Versioni e backup mentre lavori</b> (prima dei Salva, autosave periodici):<br>
<code>&lt;cartella-workspace&gt;/.izanagi/sessions/&lt;nome-area&gt;/.nebbie/versions/</code><br>
Esempio: <code>~/NebbieArcane/Aree/.izanagi/sessions/myst/.nebbie/versions/</code></li>
<li><b>Ultimo autosalvataggio</b> (quello di “Ripristina ultimo autosalvataggio”):<br>
<code>…/.izanagi/sessions/&lt;nome-area&gt;/.nebbie/workspace/</code></li>
</ul>
<p>Per tornare indietro senza aprire a mano queste cartelle: usa
<b>File → Cronologia</b> oppure <b>Ripristina da archivio</b> nel pannello Aree.</p>

<h3>Solo se usi la lib monolite</h3>
<ul>
<li><code>&lt;cartella-lib&gt;/.nebbie/versions/</code> — versioni e backup</li>
<li><code>&lt;cartella-lib&gt;/.nebbie/workspace/</code> — ultimo autosalvataggio</li>
</ul>
<p>Esempio tipico: <code>…/mudroot/lib/.nebbie/versions/</code></p>
)HTML");
}

QString menuGuideHtmlEnglish() {
    return QStringLiteral(R"HTML(
<h2>Izanagi menu guide</h2>
<p>Plain-language explanation of each menu item. The recommended workflow is the
<strong>Aree workspace</strong> (area folders), not the monolith lib.</p>

<h3>File</h3>
<ul>
<li><b>Open Aree workspace</b> — Pick the world root that contains area folders
(e.g. <code>castelli/</code>, <code>myst/</code>). Then open an area from the left list.</li>
<li><b>Open monolith lib</b> — Only if you still work on the single server lib
(<code>mudroot/lib</code> with <code>myst.*</code>). Do not use this for Aree area folders.</li>
<li><b>Reload library</b> — Reloads what you have open from disk, discarding unsaved
in-memory changes.</li>
<li><b>Save</b> — Writes changes to the open area (or lib). Creates a safety copy first.</li>
<li><b>Save (force)</b> — Saves even when warnings would normally stop the save.</li>
<li><b>History → Restore last autosave</b> — Goes back to the latest automatic snapshot.</li>
<li><b>History → Restore version…</b> — Pick an earlier snapshot (autosave or pre-save).</li>
<li><b>Exit</b> — Quit Izanagi.</li>
</ul>
<p>In the <b>Aree workspace</b> dock (left side, after opening a workspace):</p>
<ul>
<li><b>Open area</b> — Loads the selected area immediately (no dialog).</li>
<li><b>Archive area</b> — Full-folder safety copy of the open (or selected) area.
Optional: Preferences → archive on first Save.</li>
<li><b>Restore from archive</b> — Restores the area from an earlier full-folder archive
(see paths below).</li>
</ul>

<h3>Edit</h3>
<ul>
<li><b>Undo / Redo</b> — Step back and forward in text edits.</li>
<li><b>Cut / Copy / Paste</b> — Standard editing.</li>
<li><b>Paste plain text</b> — Paste text without formatting.</li>
<li><b>Select all</b> — Select all text in the active field.</li>
</ul>

<h3>Tools</h3>
<ul>
<li><b>Check open area (or lib)…</b> — Looks for errors and warnings in
<strong>what you have open now</strong>:
<ul>
<li>area from the workspace → checks <em>only that area</em>
(rooms, mobs, objects, zones, shops, etc.);</li>
<li>monolith lib → checks the whole loaded lib.</li>
</ul>
It does not check every area in the workspace at once. On save, Izanagi runs a
narrower check (mainly dirty rooms); use this item for a full check of what is open.</li>
<li><b>Export overlays…</b> — Builds server overlay files from what is open
(advanced / deploy use).</li>
<li><b>Normalize exit descriptions…</b> — Fixes exit descriptions that differ from the
destination room name only by stray spaces or newlines. Keeps custom look text.</li>
</ul>

<h3>Preferences</h3>
<ul>
<li><b>Line length limit…</b> — Characters per line in texts (MUD limits).</li>
<li><b>Extended color code view</b> — Show or hide raw color codes (e.g. <code>$cXXXX</code>).</li>
<li><b>Lingua / Language</b> — Italian or English menus.</li>
</ul>

<h3>Colors</h3>
<ul>
<li><b>MUD color legend</b> — List of in-game colors.</li>
<li><b>Insert code…</b> — Inserts a color code into the text field you are editing.</li>
</ul>

<h3>Coordinator</h3>
<p>Tools for coordinating zone number ranges (vnums) across builders.
Most day-to-day area work can ignore this menu.</p>
<ul>
<li><b>Configuration…</b> — Coordinator service address and settings.</li>
<li><b>Refresh / Load / Export world index</b> — Update, load, or export the zone index.</li>
<li><b>Reserve vnums…</b> — Reserve a number range for a new zone so it does not overlap others.</li>
</ul>

<h3>Help</h3>
<ul>
<li><b>Check for updates…</b> — See if a newer Izanagi build is available.</li>
<li><b>Check for updates on startup</b> — Do that automatically when the app starts.</li>
<li><b>Menu guide…</b> — This page.</li>
<li><b>About Izanagi…</b> — Version and credits.</li>
</ul>

<h2>Where safety saves live</h2>
<p>They sit next to the workspace (or lib) you opened. Replace the angle-bracket
parts with your real paths.</p>

<h3>Normal use — Aree workspace</h3>
<p>Example: workspace at <code>~/NebbieArcane/Aree</code>, area <code>myst</code>.</p>
<ul>
<li><b>Area archives</b> (full folder copy, e.g. when opening):<br>
<code>&lt;workspace-folder&gt;/.izanagi/archives/&lt;area-name&gt;/&lt;timestamp&gt;/</code><br>
Example: <code>~/NebbieArcane/Aree/.izanagi/archives/myst/2026-04-10_123045/</code></li>
<li><b>Versions while you work</b> (pre-save backups, periodic autosaves):<br>
<code>&lt;workspace-folder&gt;/.izanagi/sessions/&lt;area-name&gt;/.nebbie/versions/</code><br>
Example: <code>~/NebbieArcane/Aree/.izanagi/sessions/myst/.nebbie/versions/</code></li>
<li><b>Latest autosave</b> (“Restore last autosave”):<br>
<code>…/.izanagi/sessions/&lt;area-name&gt;/.nebbie/workspace/</code></li>
</ul>
<p>To restore without browsing folders by hand: use <b>File → History</b> or
<b>Restore from archive</b> in the Aree dock.</p>

<h3>Only if you use the monolith lib</h3>
<ul>
<li><code>&lt;lib-folder&gt;/.nebbie/versions/</code> — versions and backups</li>
<li><code>&lt;lib-folder&gt;/.nebbie/workspace/</code> — latest autosave</li>
</ul>
<p>Typical example: <code>…/mudroot/lib/.nebbie/versions/</code></p>
)HTML");
}

} // namespace

QString menuGuideHtml() {
    if (currentAppLanguage() == AppLanguage::English) {
        return menuGuideHtmlEnglish();
    }
    return menuGuideHtmlItalian();
}

void showMenuGuideDialog(QWidget* parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle(appTr("menu.help_guide"));
    dialog.resize(720, 560);

    auto* layout = new QVBoxLayout(&dialog);
    auto* browser = new QTextBrowser;
    browser->setOpenExternalLinks(false);
    browser->setHtml(menuGuideHtml());
    layout->addWidget(browser, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);

    dialog.exec();
}

} // namespace nebbie::qt
