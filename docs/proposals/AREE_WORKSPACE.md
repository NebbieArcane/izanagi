# Workspace Aree in Izanagi

**Branch:** `cursor/aree-workspace-92f1`  
**Stato:** P0 core + P1 GUI (lista, apertura, archivio, ripristino)  
**Build monolite:** `deploy_aree.php` in NebbieArcane/Aree (non modificabile)

---

## Decisioni confermate (2026-09-03)

| # | Domanda | Risposta |
|---|---------|----------|
| 1 | Radice workspace | Qualsiasi cartella che contiene **un livello** di sottocartelle-area (`pippo/`, `mudroot/`, `mudroot/src/` — il nome del padre non conta). |
| 2 | Nome cartella | Deve coincidere con il basename dei file (`castelli/castelli.zon`) per `deploy_aree.php`. |
| 3 | Zone per cartella | Una sola `.zon` per cartella + `.wld`, `.mob`, `.obj`, … |
| 4 | Estensioni area | `.zon`, `.wld`, `.mob`, `.obj`, `.shp`, `.spe` (`.act`/`.pos`/`.gui` sono globali al mud). File `.ele` presenti in alcune aree ma **non** inclusi nel merge PHP. |
| 5 | Dove editare | **Edit in-place** sulla cartella area + **archivio** in `.izanagi/archives/` (vedi sotto). |
| 6 | Archivio | Timestamp + commento opzionale (tipo messaggio commit). |
| 7 | Panoramica | Lista aree; mappa geografica **non** in scope. |
| 8 | Build | `build.sh` lanciato **a mano** dall’utente. |
| 9 | `.gitignore` | Solo locale (non toccare repo Aree online). |
| 10 | Fixture | `tests/fixtures/aree/castelli/`, `myst/` da campioni reali. |

### Verifica campioni

I file area reali **non** contengono `#$`, `%%` o `#0` — la zona termina con `S`, mob/obj senza `%%`. Coerente con il concat di `deploy_aree.php`.

---

## Punto 5 — Perché edit in-place (non sandbox separata)

`deploy_aree.php` legge le cartelle sotto `src/` (nel tuo caso: sotto la **radice workspace** che scegli):

```php
$fname = sprintf('src/%1$s/%1$s%2$s', $data['fname'], $ext);
```

Se editassimo solo in `.izanagi/sandboxes/<area>/`, **ogni build** richiederebbe un passo “pubblica su src” prima di `./build.sh`. Dato che il build è manuale, è più lineare:

```
<workspace>/
  castelli/              ← editi e salvi QUI (build.sh/deploy li vede subito)
  myst/
  .izanagi/
    archives/
      castelli/
        20260903T101300Z/
        20260903T101300Z-test-integrazione/
```

**Flusso “Inizia sessione di prova”:**

1. Dialogo: commento archivio opzionale (o salta).
2. Copia `castelli/` → `.izanagi/archives/castelli/<timestamp>[-label]/`.
3. `loadLib(workspace/castelli/)` — lavori sulla cartella reale.
4. **Salva** con `write_eof_markers = false` (modalità area).
5. `./build.sh` a mano quando vuoi.
6. Se serve ripartire: **Ripristina da archivio** → sovrascrive `castelli/` dalla copia.

**Autosave Izanagi:** metadati in `<workspace>/.izanagi/sessions/<area>/.nebbie/` (non dentro la cartella area), così git/build non vedono `.nebbie` tra i file area.

---

## Modello dati

```
<WorkspaceRoot>/                 ← scelto con “Apri workspace Aree…”
  <area>/                      ← area editabile (basename = nome file)
    <area>.zon
    <area>.wld
    ...
  .izanagi/                    ← gitignore locale consigliato
    archives/<area>/<id>/
    sessions/<area>/.nebbie/   ← autosave editor
    workspace.json             ← ultima area, path root
```

**Cartelle escluse dalla lista** (come deploy): `_head`, `_tail`, `chiusura`, qualsiasi nome che inizia con `_`.

**Criterio area valida:** esiste `<area>/<area>.zon`.

---

## GUI (fasi)

### P1 — Lista e apertura

- `File → Apri workspace Aree…`
- Pannello **Aree**: cartella, `#zona`, nome, top vnum
- **Apri area** / doppio click
- Dialogo **Inizia sessione di prova**: archivia (con commento opzionale) → carica area

### P2 — Ripristino e stato

- **Ripristina da archivio…** (lista timestamp + label)
- Indicatore: area con modifiche dopo ultimo archivio

### P3 — Salvataggio area

- In modalità workspace: `save_lib` con `MystSaveOptions{.write_eof_markers = false}`
- Validazione monolith su richiesta (pre-build)

---

## Core (P0 — implementato sul branch)

`nebbie-core/aree_workspace.hpp`:

- `scan_aree_areas`
- `archive_aree_area` / `restore_aree_area_from_archive`
- `list_aree_archives`

CLI prevista: `nebbiedit aree scan`, `aree archive`, `aree restore`.

Test: `tests/test_aree_workspace.cpp` su fixture `castelli` + `myst`.

---

## Fuori scope

- Modificare `deploy_aree.php` / `build.sh`
- Mappa multi-area
- Lancio automatico build
- `.act` / `.pos` / `.gui` per area

---

## Riferimenti

- `nebbie-core/src/aree_workspace.cpp`
- `nebbie-core/src/lib_io.cpp` (`discover_files_by_extension` → `area/area.zon`)
- `scripts/aree/deploy_aree.php` (copia canonica fix EOF)
- `tests/fixtures/aree/`
