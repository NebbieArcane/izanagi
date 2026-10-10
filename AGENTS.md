# AGENTS.md

Istruzioni per agenti Cursor (IDE sul NUC e Cloud Agent). Leggere questo file prima di modificare il codice o fare operazioni git.

---

## Ambiente di sviluppo principale (NUC)

- **Directory di lavoro:** `~/NebbieArcane/izanagi`
- Aprire **sempre** questa cartella in Cursor sul NUC per lo sviluppo quotidiano.
- Dopo interventi da Cloud Agent o da altro PC: `git pull origin main` in quella directory.

---

## Repository

- **Ufficiale (pubblico):** https://github.com/NebbieArcane/izanagi
- **Branch di lavoro:** `main`
- **Remote consigliato:** `origin` → `NebbieArcane/izanagi` (evitare push ridondanti su fork se non richiesto)

Il parser del server Nebbie Arcane (`NebbieArcane/Server`, `src/db.cpp`) è la specifica dei formati `myst.*`. Non inventare formati alternativi.

---

## Git e push (regola critica)

- **Non eseguire `git push`** verso GitHub (né `origin`, né `izanagi`, né fork) **salvo richiesta esplicita dell'utente** in quel turno di conversazione.
- Commit locali sul NUC sono ok quando servono al lavoro in corso.
- Niente `git push --force` né amend di commit già pushati, salvo richiesta esplicita.
- Su Cloud Agent, se l'utente autorizza il push su `NebbieArcane/izanagi` e HTTPS fallisce (token bot), usare deploy key SSH:
  ```bash
  source ./scripts/setup-izanagi-deploy-key.sh
  GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null \
    git push git@github.com:NebbieArcane/izanagi.git main:main
  ```
- Script opzionale: `./scripts/push-to-izanagi.sh` (stesso repository ufficiale).

Non aprire branch feature né release/tag manuali salvo richiesta esplicita dell'utente.

---

## Flusso prodotto: Aree prima del monolite

- **Default:** lavoro su **workspace Aree** — root con sottocartelle `castelli/`, `myst/`, … ciascuna con `area/area.zon`, `area.wld`, `area.mob`, … (layout `NebbieArcane/Aree`, vedi `docs/proposals/AREE_WORKSPACE.md`).
- **GUI:** File → **Apri workspace Aree** (scorciatoia Ctrl+O); dock Aree → apri area → edit in-place; salvataggio area con `write_eof_markers_on_save = false`.
- **Monolite** `mudroot/lib` con `myst.*`: solo se l'utente sceglie esplicitamente **Apri lib monolite**; non aprire una cartella area singola come monolite (guard in `classify_aree_lib_open_guard`).
- **Discovery file:** in `area/`, se esiste `area/area.zon`, ha priorità su `myst.zon` nella stessa cartella (`lib_io`).

---

## Build e test (NUC Ubuntu)

```bash
cd ~/NebbieArcane/izanagi
./scripts/install-deps.sh          # prima volta o dopo aggiornamento OS
./scripts/build.sh --test          # build + test prima di considerare finito un cambio
```

Binari locali: `build/nebbie-qt/izanagi` (o `nebbieedit`), `build/nebbiedit/nebbiedit`.

---

## Pacchetti installabili

| Dove | Cosa |
|------|------|
| **NUC (Linux)** | `./scripts/package-deb.sh` → `dist/izanagi_*_amd64.deb` |
| **macOS .dmg** | **Non** sul NUC. Push su `main` → GitHub Actions → scarica artifact (vedi sotto). |
| **Windows** | Idem via CI o PC Windows. |

Guide complete:

- [docs/GUIDA_PACCHETTI.md](docs/GUIDA_PACCHETTI.md) — panoramica e CI senza clone sul Mac
- [docs/GUIDA_PACCHETTI_LINUX.md](docs/GUIDA_PACCHETTI_LINUX.md)
- [docs/GUIDA_PACCHETTI_MACOS.md](docs/GUIDA_PACCHETTI_MACOS.md)
- [docs/GUIDA_PACCHETTI_WINDOWS.md](docs/GUIDA_PACCHETTI_WINDOWS.md)

Workflow GitHub: **Izanagi (release packages)**, **Izanagi & Cypher (packages)**, **Izanagi & Cypher (CI)**.

---

## Release (solo quando l'utente chiede di pubblicare)

1. Verificare test/build; commit su `main` (locale o push se autorizzato).
2. `git push origin main` (o push SSH verso `NebbieArcane/izanagi` come sopra).
3. Controllare CI:
   ```bash
   gh run list --repo NebbieArcane/izanagi --limit 5
   gh run view --repo NebbieArcane/izanagi --log-failed
   ```
4. Pacchetti: https://github.com/NebbieArcane/izanagi/releases (tag `izanagi`, `cypher`).

Se `publish-release` fallisce, vedere `scripts/publish-github-release.sh` e i log del job.

---

## Cloud Agent vs IDE NUC

| | IDE sul NUC | Cloud Agent |
|--|-------------|-------------|
| File | `~/NebbieArcane/izanagi` | VM remota, sync via `git pull` |
| Uso tipico | Feature, debug, test GUI | Push/merge/doc quando richiesto |

Dopo lavoro in Cloud: sul NUC eseguire `git pull origin main`.

---

## Riferimenti rapidi nel repo

- `README.md` — panoramica Izanagi / Cypher
- `docs/ARCHITECTURE.md` — monolite + overlay, boot server
- `docs/PLATFORM.md` — build per OS
- `docs/proposals/AREE_WORKSPACE.md` — workspace Aree, archivi `.izanagi/`
- `scripts/aree/` — deploy PHP (non modificare senza accordo)
