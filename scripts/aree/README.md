# Aree build (`deploy_aree.php`)

**Script autoritativo:** quello nel repo **NebbieArcane/Aree** (usato da `build.sh`
sulla macchina di build). Izanagi **non** costruisce il monolite e **non** deve
“migliorare” o sostituire quello script senza accordo esplicito del proprietario.

La copia in `scripts/aree/deploy_aree.php` è solo un **mirror di riferimento**
allineato ad Aree (stesso contenuto), per documentare il contratto di concat.

## Come concatena (comportamento reale)

Ordine dei chunk: `_head` → aree (ordinate per top vnum della `.zon`) → `_tail`.

Per ogni estensione in `.wld .mob .obj .shp .spe .zon`:

| Ext | Concat |
|-----|--------|
| `.wld` | `rtrim(file) + "\n"` |
| altre | contenuto grezzo (`file_get_contents`) |

La `.zon` di ogni area viene anche usata per l’indice: primo `~` = fine nome zona
(`explode('~', …, 2)`), poi il primo numero del corpo = top vnum. Il nome zona
deve restare **inline** (`Nome zona~` sulla stessa riga).

### `_head` / `_tail` (non toccarli da Izanagi)

- `_head.wld`: stanza **`#0` reale** (“La stanza dei segreti”), non un EOF.
  Descrizione già con `~` a capo. Placeholder `<VERSION>` / `<LOGNOTES>`.
- `_tail.wld`: **vuoto** — il `.wld` monolitico finisce con l’ultima area (EOF file).
- `_tail.mob` / `_tail.obj`: `#99999` + `%%~`
- `_tail.zon`: `#999999` + `$~`
- `_tail.shp` / `_tail.spe`: `$~` / `$`
- Cartelle `_…` e `chiusura` sono escluse dalla lista aree.

## Contratto per i file che Izanagi salva (area)

Izanagi edita `src/<area>/<area>.*` in-place. Per restare compatibili con questo
concat **senza strip**:

1. **Niente terminatori di fine monolite** nei file area: niente `#$`, `%%` /
   `%%~`, `$~`, `$` finale, e **niente `#0` in coda al `.wld`**  
   (`write_eof_markers_on_save = false` in workspace Aree).
2. **`#0` nel `.wld` è solo la stanza di `_head`** — un’area non deve emettere `#0`.
3. Zona: nome con tilde **inline**; blocco che termina con `S` (niente `#$`).
4. Descrizioni stanza / look uscita: `~` a capo (`OnOwnLine`) è OK e allineato a
   `_head.wld`.
5. Non modificare `_head` / `_tail` / `deploy_aree.php` / `build.sh` da Izanagi.

Se un’area contiene per errore un terminatore in coda, lo script Aree lo lascia
in mezzo al monolite e il server può troncare il boot — la protezione è nel
salvataggio Izanagi, non nello script PHP.
