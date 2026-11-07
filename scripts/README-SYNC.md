# Sync Mac ↔ VPS

## Prima volta (sul Mac)

Assicurati che SSH funzioni:

```bash
ssh vps-tailscale 'echo ok'
```

Poi **una sola volta**:

```bash
rsync -avz vps-tailscale:/home/nebbie/new-izanagi/scripts/install-mac-sync.sh /tmp/
bash /tmp/install-mac-sync.sh
```

Questo:
1. scarica lo script di sync
2. clona/aggiorna il codice in `~/src/new-izanagi`
3. installa qt/cmake se mancano
4. compila
5. crea il comando `~/bin/izanagi-sync`

## Ogni giorno

```bash
izanagi-sync               # sync + build
izanagi-sync --open        # sync + build + avvia l'app
izanagi-sync --only-open   # solo avvia (niente sync/rebuild)
izanagi-sync --no-build    # solo aggiorna i sorgenti
```

`izanagi-sync` e' un wrapper che esegue **sempre**
`~/src/new-izanagi/scripts/mac-sync-build.sh` (path da `~/.config/izanagi/sync.env`).
Quindi `izanagi-sync --open` e
`/Users/.../src/new-izanagi/scripts/mac-sync-build.sh --open` devono fare la stessa cosa.

Se vedi build diverse:
1. controlla che lo script stampi lo stesso `Local:` / `Script:`
2. se `~/bin/izanagi-sync` e' un symlink/copia vecchia, rilancia `install-mac-sync.sh`
3. `--only-open` **non** ricompila: apre l'app gia' in `build/`

Oppure direttamente:
```bash
open ~/src/new-izanagi/build/app/izanagi.app
```

Config salvata in `~/.config/izanagi/sync.env` (VPS, path remoti/locali).
