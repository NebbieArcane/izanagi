# Nebbie Arcane Editing Tools

Strumenti desktop per modificare le librerie di mondo di [Nebbie Arcane](https://github.com/NebbieArcane/Server).

Repository: **https://github.com/wizardmorgan/nebbie-arcane-editing-tools**

Il progetto è indipendente dal codice del server: non servono permessi sull'organizzazione `NebbieArcane`. Il riferimento di formato resta il codice in `NebbieArcane/Server` (`src/db.cpp`, `src/db.hpp`).

---

## I due editor

| | **Izanagi** | **Cypher** |
|---|-------------|------------|
| **Ruolo** | Editor completo del mondo | Editor leggero per tradurre i testi |
| **Eseguibile** | `nebbieedit` (Linux/macOS) · `izanagi.exe` (Windows) | `nebbie-translate` (Linux/macOS) · `cypher.exe` (Windows) |
| **Release GitHub** | tag [`izanagi`](https://github.com/wizardmorgan/nebbie-arcane-editing-tools/releases/tag/izanagi) | tag [`cypher`](https://github.com/wizardmorgan/nebbie-arcane-editing-tools/releases/tag/cypher) |
| **Config** | `nebbieedit.conf` | `nebbie-translate.conf` |

### Izanagi — editor completo

Editor grafico (Qt) per costruire e mantenere il mondo di gioco.

Funzionalità principali:

- Apertura librerie `mudroot` / `mudroot/lib` e cartelle con file per estensione (`castelli.wld`, `castelli.mob`, …)
- **File → Aggiorna libreria** (F5): ricarica dal disco la cartella aperta
- Editor di **stanze**, **mob**, **oggetti**, **zone**, **reset**
- Mappe stanze per zona e mappa mondo (zone)
- Editor **dati mondo** (shop, special, danni, social, pose, gilde)
- **Validazione** mondo, report navigabile
- **Salva**, autosalvataggio e cronologia versioni in `.nebbie/versions/`
- Esportazione overlay e divisione per zone
- Coordinator world-index (opzionale, per team multi-builder)
- Anteprima **colori MUD** nei testi, righello lunghezza riga
- Tema scuro Fusion (leggibile su Windows, Linux e macOS)

Include anche la CLI **`nebbiedit`** per script e automazione.

### Cypher — traduttore stanze

Editor minimale per chi deve tradurre descrizioni senza toccare meccaniche di gioco.

Funzionalità principali:

- Traduzione di **nome stanza**, **descrizione**, **descrizioni extra** e **testi look uscite**
- **File → Aggiorna libreria** (F5)
- Validazione mirata, autosalvataggio, colori MUD e righello riga
- Stessa logica di apertura libreria di Izanagi

---

## Download (senza compilare)

A ogni push su `master`, la CI pubblica pacchetti precompilati per **Windows**, **macOS** e **Linux**.

### Izanagi

**Release:** https://github.com/wizardmorgan/nebbie-arcane-editing-tools/releases/tag/izanagi

| Piattaforma | File | Installazione |
|-------------|------|----------------|
| **Windows** | `izanagi_*_windows_portable.zip` | Estrai → avvia **`izanagi.exe`** (incluso `nebbiedit.exe` CLI opzionale) |
| **macOS** | `izanagi_*_macos.dmg` | Trascina **`nebbieedit.app`** in Applicazioni |
| **Linux** | `izanagi_*_amd64.deb` | `sudo dpkg -i izanagi_*.deb` → `sudo apt-get install -f` |

**Primo avvio Izanagi:** File → Apri libreria → cartella `mudroot` o `mudroot/lib` → modifica → Salva.

Ogni pacchetto include un mondo di esempio (`sample-mudroot`).

### Cypher

**Release:** https://github.com/wizardmorgan/nebbie-arcane-editing-tools/releases/tag/cypher

| Piattaforma | File | Installazione |
|-------------|------|----------------|
| **Windows** | `cypher_*_windows_portable.zip` | Estrai → avvia **`cypher.exe`** |
| **macOS** | `cypher_*_macos.dmg` | Trascina **`nebbie-translate.app`** in Applicazioni |
| **Linux** | `cypher_*_amd64.deb` | `sudo dpkg -i cypher_*.deb` → `sudo apt-get install -f` |

**Primo avvio Cypher:** File → Apri libreria → `mudroot/lib` → traduci → Salva.

### macOS — app «non verificata»

Se macOS blocca l'applicazione: tasto destro → **Apri** (solo la prima volta), oppure:

```bash
xattr -cr /Applications/nebbieedit.app
# oppure
xattr -cr /Applications/nebbie-translate.app
```

Per distribuzione senza questo passaggio serve la notarizzazione Apple (`docs/MACOS_SIGNING.md`).

### Configurazione salvata

| Tool | Linux | macOS | Windows |
|------|-------|-------|---------|
| Izanagi | `~/.config/Nebbie/nebbieedit.conf` | `~/Library/Application Support/Nebbie/nebbieedit.conf` | `%APPDATA%\Nebbie\nebbieedit.conf` |
| Cypher | `~/.config/Nebbie/nebbie-translate.conf` | `~/Library/Application Support/Nebbie/nebbie-translate.conf` | `%APPDATA%\Nebbie\nebbie-translate.conf` |

Chiave principale: `lib_path=` (ultima libreria aperta).

---

## File di libreria supportati

| File | Contenuto | Izanagi | Cypher |
|------|-----------|:-------:|:------:|
| `*.zon` / `myst.zon` | Zone e reset | ✅ | — |
| `*.wld` / `myst.wld` | Stanze | ✅ | ✅ (solo testi) |
| `*.mob` / `myst.mob` | Mob | ✅ | — |
| `*.obj` / `myst.obj` | Oggetti | ✅ | — |
| `*.shp` / `myst.shp` | Negozi | ✅ | — |
| `*.spe` / `myst.spe` | Special proc | ✅ | — |
| `*.dam` / `myst.dam` | Messaggi danno | ✅ | — |
| `*.act` / `myst.act` | Social | ✅ | — |
| `*.pos` / `myst.pos` | Pose | ✅ | — |
| `*.gui` / `myst.gui` | Gilde | ✅ | — |

Le librerie possono usare prefissi diversi da `myst` (es. `castelli.wld`) se tutti i file della cartella condividono lo stesso nome base o estensione riconosciuta.

---

## Compilare dal sorgente

### Linux / macOS

```bash
./scripts/install-deps.sh
./scripts/build.sh --test
```

Binari:

```bash
./build/nebbiedit/nebbiedit info tests/fixtures      # CLI
./build/nebbie-qt/nebbieedit tests/fixtures          # Izanagi (GUI)
./build/nebbie-translator/nebbie-translate           # Cypher (GUI)
```

### Windows

```powershell
.\scripts\install-deps.ps1
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.5.3\msvc2019_64"   # adatta al tuo Qt 6
.\scripts\build.ps1 -Test
.\build\nebbiedit\Release\nebbiedit.exe info tests\fixtures
.\build\nebbie-qt\Release\izanagi.exe
.\build\nebbie-translator\Release\cypher.exe
```

Pacchetti Windows portatili:

```powershell
.\scripts\package-windows-portable.ps1           # Izanagi → dist\izanagi_*_windows_portable.zip
.\scripts\package-windows-translate.ps1          # Cypher → dist\cypher_*_windows_portable.zip
.\scripts\package-windows.ps1                    # zip + installer Inno Setup (Izanagi)
```

### Pacchetti Linux / macOS

```bash
./scripts/package-deb.sh                 # Izanagi .deb
./scripts/package-deb-translate.sh     # Cypher .deb
./scripts/package-dmg.sh               # Izanagi .dmg (macOS)
./scripts/package-dmg-translate.sh     # Cypher .dmg (macOS)
```

---

## CLI `nebbiedit`

Disponibile in Izanagi (pacchetto e build). Utile per script, CI e modifiche rapide da terminale.

```bash
nebbiedit info /path/to/lib
nebbiedit validate /path/to/lib
nebbiedit room set /path/to/lib 3001 --name "Nuova stanza" --desc "Testo"
nebbiedit mob set /path/to/lib 1 --short "Puff aggiornato" --level 30
nebbiedit obj set /path/to/lib 1 --short "Elmo nuovo" --cost 99
nebbiedit edit /path/to/lib          # sessione interattiva
nebbiedit zone list
nebbiedit convert lib roundtrip tests/fixtures /tmp/nebbie-rt
```

---

## Struttura del repository

```
nebbie-core/        Libreria C++17 (parser, modello, validazione, I/O)
nebbiedit/          CLI nebbiedit
nebbie-qt/          GUI Izanagi (target nebbieedit / izanagi.exe)
nebbie-translator/  GUI Cypher (target nebbie-translate / cypher.exe)
scripts/            Build, pacchetti, deploy
tests/              Fixture e test automatici
docs/               Documentazione tecnica
```

---

## Documentazione aggiuntiva

- [docs/PLATFORM.md](docs/PLATFORM.md) — piattaforme e packaging
- [docs/MANUALE_INSTALLAZIONE.md](docs/MANUALE_INSTALLAZIONE.md) — installazione dettagliata ([PDF](docs/MANUALE_INSTALLAZIONE.pdf))
- [docs/PROJECT_SUMMARY.md](docs/PROJECT_SUMMARY.md) — riepilogo tecnico
- [docs/ROADMAP.md](docs/ROADMAP.md) — roadmap

---

## Riferimenti

- Server ufficiale: https://github.com/NebbieArcane/Server
- Fork dati di test: https://github.com/wizardmorgan/nebbietest

```bash
./scripts/fetch-test-data.sh
./build/nebbiedit/nebbiedit info vendor/nebbietest/mudroot/lib
```

---

## Licenza

MIT
