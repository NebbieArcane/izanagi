# AGENTS.md

## Repository

- **Ufficiale (pubblico):** https://github.com/NebbieArcane/izanagi
- **Branch di lavoro:** `main`
- **Remote:** `origin` → `NebbieArcane/izanagi` (niente doppio push su fork)

## Release workflow

Dopo ogni fix su `main`:

1. `git push origin main` (oppure `./scripts/push-to-izanagi.sh` se serve la deploy key SSH in Cloud Agent)
2. Attendi CI su `NebbieArcane/izanagi`:
   ```bash
   gh run list --repo NebbieArcane/izanagi --limit 5
   gh run view --repo NebbieArcane/izanagi --log-failed   # se qualcosa fallisce
   ```
3. Verifica pacchetti su https://github.com/NebbieArcane/izanagi/releases (tag `izanagi` e `cypher`)

Workflow rilevanti: `Izanagi (release packages)`, `Cypher (release packages)`, `Izanagi & Cypher (CI)`.

Se `publish-release` fallisce, controlla `scripts/publish-github-release.sh` e i log del job.

Non aprire branch feature né release manuali salvo richiesta esplicita dell'utente.

## Build (smoke test locale)

```bash
./scripts/package-deb.sh            # Izanagi .deb
./scripts/package-deb-translate.sh  # Cypher .deb
```
