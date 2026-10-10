# Pacchetti macOS (Izanagi / Cypher)

Come ottenere il **`.dmg`** per macOS.

---

## Importante: il DMG non si crea sul NUC

Il NUC Ubuntu **non** può eseguire `package-dmg.sh` (serve macOS: `hdiutil`, app bundle). Se il codice vive solo sul NUC, hai due strade:

1. **CI GitHub** (consigliata): push da NUC → scarichi il `.dmg` dal Mac **senza** clonare il repo.
2. **Mac con clone minimo**: sul Mac fai solo `git clone` + uno script (il sorgente non deve risiedere sul NUC in quel momento).

---

## Metodo A — DMG senza repository sul Mac (solo browser o `gh`)

Ideale quando sviluppi sul **NUC** e sul Mac vuoi solo il pacchetto.

### Passi

1. Sul **NUC**, integra e pusha su `NebbieArcane/izanagi` branch `main` (o il branch che deve essere rilasciato).
2. Su **GitHub** → [NebbieArcane/izanagi Actions](https://github.com/NebbieArcane/izanagi/actions).
3. Attendi il workflow **Izanagi (release packages)** (parte su ogni push a `main`) oppure avvialo manualmente:
   - **Izanagi (release packages)** → **Run workflow** → branch `main` → **Run workflow**.
4. Apri la run completata (tutti i job verdi, incluso **macos-dmg**).
5. Sezione **Artifacts** → scarica l’artifact macOS (es. `nebbie-editor-macos-dmg` o `nebbie-editor-dmg-macos`).
6. Sul Mac: decomprimi se necessario, apri `izanagi_*_macos.dmg`, trascina **Izanagi.app** in Applicazioni.

### Da terminale Mac (senza clone del progetto)

Con [GitHub CLI](https://cli.github.com/) loggato con **il tuo** account:

```bash
gh run list --repo NebbieArcane/izanagi --workflow "Izanagi (release packages)" --limit 3
gh run download <ID_RUN> --repo NebbieArcane/izanagi
```

Controlla i nomi degli artifact nella run (`gh run view <ID> --repo NebbieArcane/izanagi`).

### Release

Gli asset possono comparire anche su [Releases → tag izanagi](https://github.com/NebbieArcane/izanagi/releases/tag/izanagi) se il job di publish del workflow release è andato a buon fine.

---

## Metodo B — Build DMG sul Mac (clone sul Mac, non copia dal NUC)

Il Mac **non** legge il working tree del NUC via rete: clona da GitHub (stesso commit che hai pushato dal NUC).

### 1. Prerequisiti

```bash
xcode-select --install
# Homebrew: https://brew.sh
cd ~/src
git clone https://github.com/NebbieArcane/izanagi.git
cd izanagi
git checkout main
git pull
./scripts/install-deps.sh
```

Aggiungi a `~/.zshrc`:

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

### 2. Un comando per il DMG

```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
./scripts/package-dmg.sh
```

Output: **`dist/izanagi_<versione>_macos.dmg`**

Cypher:

```bash
./scripts/package-dmg-translate.sh
```

### 3. Opzioni

| Comando | Uso |
|---------|-----|
| `./scripts/package-dmg.sh --no-build` | Salta compile se `build/nebbie-qt/Izanagi.app` esiste già |
| `NEBBIE_VERSION=0.2.0 ./scripts/package-dmg.sh` | Versione fissa |

### 4. Prima installazione / Gatekeeper

Se macOS blocca l’app non notarizzata: tasto destro su **Izanagi.app** → **Apri**, oppure:

```bash
xattr -cr /Applications/Izanagi.app
```

---

## Contenuto del DMG

- `Izanagi.app`
- `bin/nebbiedit`
- `sample-mudroot/`
- collegamento **Applications**
- `LEGGIMI.txt`

---

## Checklist rapida

**Solo NUC + Mac senza sorgente**

- [ ] Push `main` su `NebbieArcane/izanagi`
- [ ] Actions → run verde → download artifact macOS
- [ ] Installa da DMG

**Mac con build locale**

- [ ] CLT + Homebrew + `qt@6`
- [ ] `git clone` izanagi, `package-dmg.sh`
- [ ] DMG in `dist/`

---

## Riferimenti

- [GUIDA_PACCHETTI.md](GUIDA_PACCHETTI.md) — panoramica multi-OS
- [PLATFORM.md](PLATFORM.md) — toolchain macOS (`check-macos-toolchain.sh`)
