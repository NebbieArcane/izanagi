# Pacchetti Windows (Izanagi / Cypher)

Come creare **zip portatile** e **installer `.exe`** per Windows.

Non è possibile generare questi pacchetti sul **NUC Linux** o su **macOS** con gli script ufficiali: serve **Windows** con MSVC e Qt 6, oppure la **CI GitHub**.

---

## Metodo A — CI GitHub (senza Visual Studio in locale)

Come per il DMG macOS: sviluppi sul NUC, push su `main`, scarichi gli artifact Windows.

1. Push su [NebbieArcane/izanagi](https://github.com/NebbieArcane/izanagi) `main`.
2. Actions → **Izanagi (release packages)** o **Izanagi & Cypher (packages)**.
3. Job **windows** / **package-windows-zip** completato.
4. Scarica artifact, ad esempio:
   - `nebbie-editor-zip-windows` → `izanagi_*_windows_portable.zip`
   - `nebbie-editor-installer-windows` → `izanagi_*_windows_setup.exe`

**Run workflow** manuale: stesso workflow → **Run workflow** → branch `main`.

Da Mac/Linux con `gh`:

```bash
gh run list --repo NebbieArcane/izanagi --workflow "Izanagi (release packages)" --limit 3
gh run download <RUN_ID> --repo NebbieArcane/izanagi
```

---

## Metodo B — Build su PC Windows

### 1. Prerequisiti

- [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/) — workload **Desktop development with C++**
- [CMake](https://cmake.org/download/)
- [Qt 6](https://www.qt.io/download-open-source) — kit **MSVC 64-bit** (es. `6.5.x msvc2019_64`)
- Per l’installer: [Inno Setup 6](https://jrsoftware.org/isinfo.php) (o `choco install innosetup`)

Dalla root del repo (PowerShell):

```powershell
.\scripts\install-deps.ps1
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.5.3\msvc2019_64"   # adatta al tuo percorso Qt
```

### 2. Clone (stesso commit del NUC)

```powershell
git clone https://github.com/NebbieArcane/izanagi.git
cd izanagi
git checkout main
git pull
.\scripts\fetch-test-data.ps1   # opzionale, come CI
```

### 3. Pacchetti Izanagi

**Solo zip portatile** (`izanagi.exe`, `nebbiedit.exe`, DLL Qt):

```powershell
.\scripts\package-windows-portable.ps1
```

**Zip + installer Inno Setup**:

```powershell
.\scripts\package-windows.ps1
```

**Solo installer** (se lo zip esiste già):

```powershell
.\scripts\package-windows-installer.ps1
```

Output in **`dist\`**:

- `izanagi_<version>_windows_portable.zip`
- `izanagi_<version>_windows_setup.exe` (se Inno Setup installato)

Cypher:

```powershell
.\scripts\package-windows-translate.ps1
```

### 4. Test rapido senza pacchetto

```powershell
.\scripts\build.ps1 -Test
.\build\nebbie-qt\Release\izanagi.exe
.\build\nebbiedit\Release\nebbiedit.exe info tests\fixtures
```

### 5. Opzioni

| Script | `--no-build` / `-NoBuild` |
|--------|-------------------------|
| `package-windows-portable.ps1` | `-NoBuild` |
| Ricompilazione | `.\scripts\build.ps1` prima del package |

Versione: variabile d’ambiente o logica in `scripts/nebbie-version.ps1` (allineata a Linux).

---

## Config utente Windows

Dopo installazione GUI:

`%APPDATA%\Nebbie\nebbieedit.conf`

---

## Problemi comuni

| Problema | Soluzione |
|----------|-----------|
| Qt non trovato | Imposta `CMAKE_PREFIX_PATH` sul root del kit Qt MSVC |
| Inno Setup mancante | Solo zip con `package-windows-portable.ps1`; installer richiede ISCC |
| Unicode path | Apri lib dalla GUI; il core usa API wide su Windows |

Vedi [PLATFORM.md](PLATFORM.md).

---

## Riferimenti

- [GUIDA_PACCHETTI.md](GUIDA_PACCHETTI.md)
- [GUIDA_PACCHETTI_MACOS.md](GUIDA_PACCHETTI_MACOS.md) — stesso schema CI per DMG senza sorgente locale
