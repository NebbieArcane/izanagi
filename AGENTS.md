# AGENTS.md

## Release workflow

After implementing a fix on `main`, always:

1. Push to `NebbieArcane/izanagi` with `./scripts/push-to-izanagi.sh` (triggers Izanagi + Cypher release workflows on push to `main`).
2. Wait for GitHub Actions on `NebbieArcane/izanagi` (`Izanagi (release packages)` and `Cypher (release packages)`) to finish.
3. Confirm packages are updated on https://github.com/NebbieArcane/izanagi/releases (tags `izanagi` and `cypher`).

Do **not** open feature branches or publish releases unless the user explicitly asks otherwise.

## Git

- Work on `main` by default.
- Use `./scripts/push-to-izanagi.sh` for the org repo (deploy key `IZANAGI_DEPLOY_KEY`).

## Build (local smoke test)

```bash
./scripts/package-deb.sh          # Izanagi .deb
./scripts/package-deb-translate.sh  # Cypher .deb
```
