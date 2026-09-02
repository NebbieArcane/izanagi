# Campioni aree Aree (per test e revisione)

Per far verificare a Izanagi / al Cloud Agent come sono scritti i file di area singola nel repo **Aree**:

1. Copia **una** cartella `src/<nome-area>/` (tutti i file `.zon`, `.wld`, `.mob`, `.obj`, …) qui sotto, es.:
   ```
   tests/fixtures/aree/<nome-area>/<nome-area>.zon
   tests/fixtures/aree/<nome-area>/<nome-area>.wld
   ...
   ```

2. Oppure incolla in chat le **ultime 15 righe** di ciascun file (soprattutto `.zon`, `.mob`, `.obj`, `.wld`) per vedere se ci sono `#$`, `%%`, `#0`.

3. Oppure apri una PR su `izanagi` con solo quella cartella in `tests/fixtures/aree/`.

## Cosa deve contenere un file area (per `deploy_aree.php`)

| File | OK | Da evitare nel body |
|------|-----|---------------------|
| `.zon` | `#N` + reset comandi | `#$` (solo in `_tail`) |
| `.mob` / `.obj` | voci `#vnum` | `%%` / `%%~` |
| `.wld` | stanze `#vnum` … `S` | `#0` (solo in `_tail`) |

Izanagi **Salva libreria** su `mudroot/lib` scrive il monolite completo **con** terminatori finali (corretto per il server).

**Dividi per zone** / `nebbiedit zone split` scrive pacchetti **senza** terminatori (compatibile Aree). Usa `--aree-layout` per nomi `slug/slug.zon` come in `src/<area>/`.
