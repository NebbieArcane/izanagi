## Izanagi

Editor completo del mondo Nebbie (izanagi / nebbieedit): stanze, mob, oggetti, zone, mappe, dati mondo e validazione.

Scarica il pacchetto per la tua piattaforma (nessuna compilazione).

| Piattaforma | File |
|-------------|------|
| **Windows** | `izanagi_*_windows_portable.zip` — estrai e avvia `izanagi.exe` |
| **macOS** | `izanagi_*_macos.dmg` — trascina `Izanagi.app` in Applicazioni |
| **Linux** | `izanagi_*_amd64.deb` — `sudo dpkg -i …` poi `sudo apt-get install -f` |

**macOS — primo avvio:** se macOS dice che l'app è «danneggiata», non è il download corrotto: è Gatekeeper. Tasto destro → **Apri** (prima volta), oppure `xattr -cr /Applications/Izanagi.app` (vedi `LEGGIMI.txt` nel DMG).
Per aprirla senza questo passaggio serve la notarizzazione Apple (`docs/MACOS_SIGNING.md`).

### Primo avvio
1. Avvia **Izanagi** (`izanagi.exe`)
2. **File → Apri libreria** → cartella `mudroot` o `mudroot/lib`
3. Modifica e **Salva**

Ogni pacchetto include un mondo di esempio (`sample-mudroot`).

### Config
- Linux: `~/.config/Nebbie/nebbieedit.conf`
- macOS: `~/Library/Application Support/Nebbie/nebbieedit.conf`
- Windows: `%APPDATA%\Nebbie\nebbieedit.conf`
