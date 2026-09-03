# Proposta: workspace Aree in Izanagi

**Branch:** `cursor/aree-workspace-92f1`  
**Stato:** bozza per revisione — nessuna implementazione UI in questa fase  
**Contesto:** repo [NebbieArcane/Aree](.) con `src/<area>/<area>.*` e build monolitico via `deploy_aree.php` (non modificabile dall’utente).

---

## 1. Problema

Il flusso di editing oggi è frammentato:

| Fase | Dove | Formato |
|------|------|---------|
| Sorgente versionato | `Aree/src/<nome>/` | `<nome>.zon`, `.wld`, `.mob`, … (senza terminatori EOF) |
| Monolite runtime | `mudroot/lib/myst.*` | file unici con `#$` / `%%` / `#0` solo in coda |
| Editing Izanagi attuale | una cartella “libreria” alla volta | `myst.*` o overlay; **Apri libreria** non conosce il repo Aree |

L’utente vuole:

1. **Vedere** da una cartella radice tutte le sottocartelle-area disponibili e **selezionarne una** per lavorarci con la visione d’insieme di Izanagi (mappa, liste, editor).
2. **Archiviare** l’area selezionata prima di modificare: copia esatta dell’originale, poi lavoro su una **copia di lavoro** così da poter ripetere il flusso integrazione (edit → monolite → test Aree) senza sporcare l’originale in `src/`.

---

## 2. Cosa esiste già in Izanagi (riuso)

| Pezzo | Ruolo per Aree |
|-------|----------------|
| `load_lib` + `discover_files_by_extension` | Carica già `area/area.zon` (non solo `myst.zon`) |
| `MystSaveOptions.write_eof_markers = false` | Salvataggio compatibile concat `deploy_aree.php` |
| `export_zone_pack` + `--aree-layout` | Export verso layout `slug/slug.*` |
| `merge_zone_packs` | Ricostruisce monolite da pacchetti (CLI) |
| `backup_lib_on_disk` | Copia file lib su disco (usato pre-salvataggio) |
| `.nebbie/versions/` | Snapshot per **una** libreria aperta |

**Gap principali:** nessuna scansione `src/`, nessun concetto “repo Aree”, nessun sandbox, salvataggio libreria sempre con EOF, versioning sotto la cartella area (inquinerebbe git).

---

## 3. Modello proposto: **Workspace Aree**

### 3.1 Concetti

```
<AreeRoot>/                    ← es. clone NebbieArcane/Aree
  src/
    <area>/                    ← originale versionato (sola lettura in modalità sandbox)
      <area>.zon
      <area>.wld
      ...
    _head/  _tail/  chiusura/  ← esclusi dalla lista aree (come deploy_aree.php)
  .izanagi/                    ← NUOVO: metadati e sandbox (gitignore consigliato)
    workspace.json
    archives/<area>/<timestamp>/   ← copie “congelate” dell’originale
    sandboxes/<area>/              ← copia di lavoro attiva
```

- **Aree root:** cartella radice del repo (o almeno `src/`).
- **Area:** sottocartella di `src/` con almeno un `.zon` (criterio allineato a `deploy_aree.php`).
- **Originale:** `src/<area>/` — non modificato finché l’utente non fa **Pubblica su Aree** (esplicito).
- **Sandbox:** `/.izanagi/sandboxes/<area>/` — dove Izanagi carica/salva durante l’editing.
- **Archivio:** `/.izanagi/archives/<area>/<timestamp>/` — snapshot immutabile dell’originale (o della sandbox) al momento dell’azione “Inizia sessione di prova”.

### 3.2 Perché sandbox fuori da `src/`

- Evita commit accidentali di `.nebbie/` dentro `src/<area>/`.
- L’originale in git resta intatto fino a publish volontario.
- Coerente con il desiderio “rifare da capo il flusso integrazione” (si elimina/ricrea la sandbox dall’archivio).

**Alternativa scartata (per ora):** edit in-place su `src/<area>/` con solo backup in archivio — più semplice ma rischio sporcare l’originale al primo salvataggio.

---

## 4. Esperienza utente (GUI)

### 4.1 Apertura workspace

**Menu:** `File → Apri workspace Aree…`  
Scegli la cartella radice del repo (es. `~/NebbieArcane/Aree`).

Pannello laterale (o dock) **Aree** con tabella:

| Colonna | Contenuto |
|---------|-----------|
| Cartella | nome directory (`src/foo` → `foo`) |
| Zona | `#N` da `.zon` se leggibile |
| Nome zona | da `.zon` |
| Stanze | conteggio `#` in `.wld` (opzionale, scan leggero) |
| Stato | `originale` / `sandbox attiva` / `modifiche non pubblicate` |

Filtri: nascondi `_head`, `_tail`, `chiusura`, cartelle che iniziano con `_`.

### 4.2 Selezione area

Doppio click o **Apri area** sulla riga:

1. Se esiste sandbox con modifiche non salvate → chiedi conferma.
2. Dialogo **Come vuoi lavorare?**
   - **A) Usa sandbox esistente** (continua `/.izanagi/sandboxes/<area>/`)
   - **B) Nuova sessione di prova** (consigliato per test integrazione):
     - Copia `src/<area>/` → `archives/<area>/<timestamp>/` (archivio)
     - Copia archivio → `sandboxes/<area>/` (sovrascrive sandbox)
     - `loadLib(sandboxes/<area>/)`
   - **C) Solo lettura** — carica originale senza sandbox (ispeziona senza salvare)

Barra stato: `Workspace: Aree | Area: foo | Modalità: sandbox | Origine: src/foo`

### 4.3 Durante l’editing

- **Salva** → scrive in `sandboxes/<area>/` con `write_eof_markers = false`.
- **Valida** → come oggi + check monolith se l’utente esporta monolite.
- Autosave/versioning → sotto `sandboxes/<area>/.nebbie/` (non in `src/`).

### 4.4 Azioni verso Aree / integrazione

| Azione | Effetto |
|--------|---------|
| **Pubblica su Aree** | Copia `sandboxes/<area>/` → `src/<area>/` (conferma + diff opzionale) |
| **Ripristina da originale** | Ricopia `src/<area>/` → sandbox (dopo conferma) |
| **Ripristina da archivio…** | Scegli timestamp in `archives/<area>/` → ricrea sandbox |
| **Esporta monolite di prova** | Merge locale o copia in `mudroot/lib` per test server (wizard) |
| **Simula build Aree** | Opzionale: invoca path noto di `deploy_aree.php` se configurato |

### 4.5 Visione d’insieme

Due livelli:

1. **Lista aree** (sempre visibile con workspace aperto) — panoramica repo.
2. **Area aperta** — UI attuale Izanagi (mappe, room/mob/obj) sul contenuto della sandbox.

Estensione futura: mappa “mondo” con tutte le aree posizionate per vnum range (da `aree.index` o da scan `.zon`) — **non in fase 1**.

---

## 5. Modulo core (nebbie-core)

Nuovo header `aree_workspace.hpp` / `aree_workspace.cpp`:

```cpp
struct AreeAreaInfo {
    std::filesystem::path folder_name;
    std::filesystem::path source_path;      // src/<area>
    std::filesystem::path sandbox_path;     // .izanagi/sandboxes/<area>
    int zone_num = 0;
    std::string zone_name;
    bool has_sandbox = false;
    bool sandbox_dirty = false;             // vs last archive/origin
};

struct AreeWorkspace {
    std::filesystem::path root;             // Aree repo root
    std::filesystem::path src_dir;          // root / "src"
    std::filesystem::path izanagi_meta;     // root / ".izanagi"
};

std::vector<AreeAreaInfo> scan_aree_workspace(const AreeWorkspace& ws);
bool is_aree_reserved_folder(const std::string& name);  // _head, _tail, chiusura, _*
std::filesystem::path archive_area(const AreeWorkspace&, const std::string& area);
bool create_sandbox_from_source(const AreeWorkspace&, const std::string& area);
bool publish_sandbox_to_source(const AreeWorkspace&, const std::string& area);
```

CLI parallela: `nebbiedit aree scan`, `aree sandbox`, `aree publish`.

---

## 6. Configurazione

Estendere `AppConfig`:

```ini
aree_root=/path/to/Aree
aree_last_area=foo
# opzionale:
aree_deploy_script=/path/to/Aree/build.sh
```

`lib_path` resta per chi apre ancora `mudroot/lib` in modo classico.

---

## 7. Fasi di implementazione

| Fase | Contenuto | Deliverable |
|------|-----------|-------------|
| **P0** | Core scan + sandbox copy + save senza EOF in modalità area | `nebbie-core`, test con `tests/fixtures/aree/` |
| **P1** | Pannello GUI Aree + dialog “Nuova sessione di prova” | `MainWindow` dock |
| **P2** | Pubblica / Ripristina / lista archivi | Azioni menu |
| **P3** | Wizard “test integrazione” (export monolite + hint build) | Documentazione operativa |
| **P4** | Mappa multi-area (opzionale) | Dopo feedback |

---

## 8. Domande aperte (servono risposte prima di implementare)

1. **Percorso radice:** apri sempre la root del repo Aree (`~/NebbieArcane/Aree`) o direttamente `src/`?
2. **Nome cartella area:** deve restare **identico** al basename file (`src/foo/foo.zon`)? (Il build PHP lo richiede.)
3. **Una zona per cartella:** è sempre così, o esistono cartelle con più `.zon`?
4. **File per area:** oltre `.zon/.wld/.mob/.obj`, servono in editing `.shp`, `.spe`, `.dam`, `.act`, `.pos`, `.gui`?
5. **Sandbox:** confermi il modello **copia in `.izanagi/sandboxes/`** con originale intatto fino a “Pubblica”? O preferisci edit diretto su `src/` + solo archivio timestamp?
6. **Archivio:** basta copia cartella timestamp, o vuoi etichetta/note (es. “prima test build 2026-09-03”)?
7. **Visione d’insieme:** in fase 1 ti basta la **lista aree** con metadati zona, o serve subito anche una vista mappa globale?
8. **Test integrazione:** dopo edit in sandbox, il flusso tipico è copiare manualmente in `src/` e lanciare `./build.sh`, o vuoi che Izanagi lanci `build.sh` / apra `mudroot/myst.zip`?
9. **`.izanagi/` vs `.nebbie/`:** va aggiunto `.izanagi/` al `.gitignore` di Aree? (consigliato sì.)
10. **Campioni:** puoi mettere 1–2 cartelle reali in `tests/fixtures/aree/` su izanagi per sviluppo e test?

---

## 9. Rischio e mitigazioni

| Rischio | Mitigazione |
|---------|-------------|
| Salvataggio con `%%` in sandbox poi publish su `src/` | `write_eof_markers = false` forzato in modalità Aree |
| Slug Izanagi ≠ nome cartella Aree | In workspace **non** usare `aree_area_slug` per il path; usare sempre il nome cartella reale |
| `.nebbie` in git | Sandbox e versioni solo sotto `.izanagi/sandboxes/` |
| Area senza `.zon` | Escludere dalla lista o mostrare come “non valida” |

---

## 10. Fuori scope (esplicito)

- Modificare `deploy_aree.php` o `build.sh` in Aree.
- Sostituire git come sistema di versioning del repo Aree.
- Editing simultaneo di più aree in una sola `World` (resta un’area per sessione).

---

## Riferimenti codice attuale

- Caricamento lib: `nebbie-core/src/lib_io.cpp` (`discover_files_by_extension`)
- Backup disco: `nebbie-core/src/session.cpp` (`backup_lib_on_disk`)
- Export zone Aree: `nebbie-core/src/zone_partition.cpp` (`aree_layout`, `write_eof_markers`)
- GUI lib: `nebbie-qt/main_window.cpp` (`openLibPath`, `loadLib`, `exportZonePacks`)
