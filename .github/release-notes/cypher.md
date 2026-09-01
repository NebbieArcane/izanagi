## Cypher

Editor leggero per tradurre **nome stanza**, **descrizione**, **descrizioni extra** e **testi look uscite** (cypher / nebbie-translate).

Scarica il pacchetto per la tua piattaforma (nessuna compilazione).

| Piattaforma | File |
|-------------|------|
| **Windows** | `cypher_*_windows_portable.zip` — estrai e avvia `cypher.exe` |
| **macOS** | `cypher_*_macos.dmg` — trascina `Cypher.app` in Applicazioni |
| **Linux** | `cypher_*_amd64.deb` — `sudo dpkg -i …` poi `sudo apt-get install -f` |

**macOS — primo avvio:** se macOS dice che l'app è «danneggiata», non è il download corrotto: è Gatekeeper. Tasto destro → **Apri** (prima volta), oppure `xattr -cr /Applications/Cypher.app` (vedi `LEGGIMI.txt` nel DMG).
Per aprirla senza questo passaggio serve la notarizzazione Apple (`docs/MACOS_SIGNING.md`).

### Primo avvio
1. Avvia **Cypher** (`cypher.exe`)
2. **File → Apri libreria** → cartella `mudroot` o `mudroot/lib`
3. Modifica i testi → **Salva**

Ogni pacchetto include un mondo di esempio (`sample-mudroot`).

### Config
- Linux: `~/.config/Nebbie/nebbie-translate.conf`
- macOS: `~/Library/Application Support/Nebbie/nebbie-translate.conf`
- Windows: `%APPDATA%\Nebbie\nebbie-translate.conf`
