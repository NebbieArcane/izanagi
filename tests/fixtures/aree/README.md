# Campioni aree Aree (per test e revisione)

Fixture reali: `castelli/`, `myst/` (zone #340 e #30). Nessun terminatore EOF nei file area.

Per far verificare altre aree al Cloud Agent:

1. Copia `src/<nome-area>/` qui sotto, oppure
2. Allega uno zip in chat (come `castelli.zip`), oppure
3. Incolla le ultime righe di `.zon` / `.mob` / `.obj` / `.wld`

## Layout richiesto da deploy_aree.php

```
<workspace>/
  <area>/
    <area>.zon
    <area>.wld
    <area>.mob
    <area>.obj
    <area>.shp   (opzionale)
    <area>.spe   (opzionale)
```

Il nome della cartella **deve** coincidere con il prefisso dei file.

## Cosa NON deve esserci nei file area

| File | Evitare |
|------|---------|
| `.zon` | `#$` |
| `.mob` / `.obj` | `%%` / `%%~` |
| `.wld` | `#0` |

Izanagi in modalità workspace salva senza questi marker; il monolite li riceve solo da `_tail` al build.

