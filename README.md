# Eleutheria (Nebbie Arcane)

Editor di aree Diku/DaleMUD rifatto da zero per **Windows**, **macOS** e **Linux**, adattato al formato mondo di [Nebbie Arcane](https://github.com/NebbieArcane/Server).

Ispirato a Eleutheria di Lavinio Cerquetti (solo UI/workflow: area `.ele` + compagni `.wld/.mob/.obj/.zon/.shp`). Il binario originale non aveva sorgenti; questo progetto e' un rewrite C++17 + Qt 6.

## Requisiti

- CMake >= 3.21
- Compilatore C++17 (GCC, Clang o MSVC)
- Qt 6 (Widgets)

## Build

```bash
./scripts/build.sh
```

Su **macOS** prima: `brew install qt cmake` (lo script trova Qt da Homebrew da solo).

Dettagli per OS: [docs/PLATFORM.md](docs/PLATFORM.md).

Binario: `build/app/eleutheria` (Linux), `eleutheria.exe` (Windows), `eleutheria.app` (macOS).

## Uso

1. Avvia Eleutheria
2. **File → Apri area** e scegli un `.ele` / `.wld` (carica tutti i file compagni)
3. Modifica stanze, mob, oggetti, zone
4. **File → Salva**

Nella cartella `sample/B1` c'e' un'area di esempio Nebbie Arcane.

## Formato

Allineato a `NebbieArcane/Server/src/db.cpp`:

- numeri con flag OR stile `2|4|32`
- stringhe terminate da `~`
- mob tipi `A/L/N/B/S`
- zone con comandi `M/O/G/E/P/D/H/...`
- metadati area in `.ele` (`Nome`, `Autore`, `MudId=nebbie`)
