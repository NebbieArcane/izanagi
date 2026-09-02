# Aree build (`deploy_aree.php`)

Il repo **NebbieArcane/Aree** (privato) costruisce `mudroot/myst.*` concatenando
`src/<area>/<area>.{zon,wld,mob,obj,...}` via `build.sh` → `deploy_aree.php`.

## Bug merge (terminatori prematuri)

Se un file di area singola termina con i marker EOF del server (`#$`, `%%`, `%%~`, `#0`)
— tipico dopo export Izanagi zone-pack o copia da monolite — `deploy_aree.php` li
concatenava così com’erano. Il server NebbieArcane **si ferma al primo terminatore** e
carica solo una parte di zone/mob/obj.

## Fix

`scripts/aree/deploy_aree.php` in questo repo è la versione corretta: per ogni area
(non `_tail`) rimuove i marker EOF prima del concat. `_tail` resta l’unica fonte dei
terminatori finali.

Copia in Aree:

```bash
cp scripts/aree/deploy_aree.php /path/to/Aree/deploy_aree.php
```

Verifica post-build:

```bash
nebbiedit validate mudroot/lib   # dopo unzip in lib
# oppure
python3 scripts/audit-myst-lib.py mudroot/lib
```

## TUNNEL moblim in myst.wld

Le righe standalone `1`/`2`/`3` dopo stanze TUNNEL vanno corrette nelle aree sorgente
o con `nebbiedit repair-lib` sul monolite — non sono risolte dal solo strip EOF.
