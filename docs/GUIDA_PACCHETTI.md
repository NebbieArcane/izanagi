# Guida pacchetti Izanagi e Cypher

Come creare i pacchetti installabili (.deb, .dmg, .zip / .exe) per **Linux**, **macOS** e **Windows**.

Repository ufficiale: [NebbieArcane/izanagi](https://github.com/NebbieArcane/izanagi).

---

## Quale guida aprire

| Sistema | Documento | Output tipico |
|---------|-----------|----------------|
| **Linux** (Ubuntu/Debian, anche il NUC) | [GUIDA_PACCHETTI_LINUX.md](GUIDA_PACCHETTI_LINUX.md) | `dist/izanagi_*_amd64.deb` |
| **macOS** | [GUIDA_PACCHETTI_MACOS.md](GUIDA_PACCHETTI_MACOS.md) | `dist/izanagi_*_macos.dmg` |
| **Windows** | [GUIDA_PACCHETTI_WINDOWS.md](GUIDA_PACCHETTI_WINDOWS.md) | `dist/izanagi_*_windows_portable.zip`, `*_windows_setup.exe` |

Cypher (solo traduttore): stessi script con suffisso `-translate` (`package-deb-translate.sh`, `package-dmg-translate.sh`, `package-windows-translate.ps1`).

---

## Dove può girare ogni build

| Pacchetto | Serve questa macchina | Si può fare sul NUC Ubuntu? |
|-----------|---------------------|----------------------------|
| `.deb` Linux | Linux (Debian/Ubuntu) | **Sì** |
| `.dmg` macOS | macOS (`hdiutil`, bundle Qt) | **No** |
| `.zip` / installer Windows | Windows + MSVC + Qt | **No** |

Il **NUC** è il posto giusto per sviluppare e per il `.deb`. Per **DMG** e **Windows** usa un Mac/PC dedicato **oppure** la CI GitHub (vedi sotto): non serve copiare il tree di sviluppo dal NUC sul Mac.

---

## Flusso consigliato: codice sul NUC, pacchetti per tutti gli OS

1. **Sul NUC:** lavori su `main` (o branch), test (`./scripts/build.sh --test`), commit.
2. **Push** su `NebbieArcane/izanagi` (es. `./scripts/push-to-izanagi.sh` con deploy key, o `git push` verso `izanagi`).
3. **GitHub Actions** compila su runner Linux / macOS / Windows e pubblica gli artifact (e, su `main`, il workflow release).

Non devi avere il repository sul Mac per ottenere il `.dmg`: basta il push dal NUC e il download dell’artifact (o della release).

### Scaricare il DMG senza codice sul Mac

1. Apri [Actions — NebbieArcane/izanagi](https://github.com/NebbieArcane/izanagi/actions).
2. Scegli un workflow completato con successo:
   - **Izanagi (release packages)** — consigliato su `main`;
   - oppure **Izanagi & Cypher (packages)**.
3. Apri l’ultima esecuzione relativa al commit che ti interessa.
4. In **Artifacts**, scarica il job macOS (nome tipo `nebbie-editor-dmg-macos` o `nebbie-editor-macos-dmg` a seconda del workflow).
5. Estrai / apri il `.dmg` dall’archivio scaricato.

**Avvio manuale** (senza nuovo push): nella pagina Actions → workflow **Izanagi (release packages)** → **Run workflow** → branch `main` → Run. Attendi i job verdi e scarica l’artifact macOS.

Da terminale (con `gh` autenticato sul tuo account, non sul NUC):

```bash
gh run list --repo NebbieArcane/izanagi --workflow "Izanagi (release packages)" --limit 5
gh run download <RUN_ID> --repo NebbieArcane/izanagi -n nebbie-editor-macos-dmg
```

(I nomi artifact possono variare leggermente tra i workflow; controlla la lista nella run.)

### Release pubbliche

Dopo push su `main`, il workflow **Izanagi (release packages)** può anche aggiornare gli asset sul tag/release [izanagi](https://github.com/NebbieArcane/izanagi/releases/tag/izanagi) (se configurato nel job di publish). In alternativa usa sempre gli **Artifacts** della run.

---

## Build locale (riassunto)

| OS | Comando principale |
|----|-------------------|
| Linux | `./scripts/package-deb.sh` |
| macOS | `export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"` poi `./scripts/package-dmg.sh` |
| Windows | `.\scripts\package-windows.ps1` (zip + installer) |

Opzione comoda su Linux/macOS: `./scripts/package.sh` (su Linux chiama `package-deb.sh`, su macOS `package-dmg.sh`).

Versione pacchetto: da `CMakeLists.txt` / `build/generated/version.hpp`, oppure:

```bash
export NEBBIE_VERSION=0.2.0
./scripts/package-deb.sh
```

Salta la ricompilazione se `build/` è già pronto: `--no-build` (dove supportato).

---

## Riferimenti

- [PLATFORM.md](PLATFORM.md) — requisiti e troubleshooting
- [MANUALE_INSTALLAZIONE.md](MANUALE_INSTALLAZIONE.md) — installazione per gli utenti finali
- Script: `scripts/package-*.sh` / `scripts/package-*.ps1`
