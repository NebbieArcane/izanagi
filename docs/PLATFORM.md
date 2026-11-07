# Build multi-piattaforma

## Linux
```bash
./scripts/build.sh
./build/app/izanagi
```
Dipendenze tipiche Debian/Ubuntu: `qt6-base-dev cmake g++`

## macOS

### Sync + build automatico (consigliato)

Sul Mac, una volta sola:
```bash
# copia lo script dal VPS oppure clona/rsync iniziale
mkdir -p ~/src
rsync -avz --exclude build --exclude izanagi-src \
  USER@HOST:/home/nebbie/new-izanagi/ ~/src/new-izanagi/
```

Poi ogni volta che aggiorni il codice sul VPS:
```bash
export IZANAGI_VPS="USER@HOST"
export IZANAGI_REMOTE="/home/nebbie/new-izanagi"
export IZANAGI_LOCAL="$HOME/src/new-izanagi"
~/src/new-izanagi/scripts/mac-sync-build.sh
```

Lo script: rsync dal VPS → installa qt/cmake se manca → `./scripts/build.sh` → toglie quarantine.

### Alternativa: tarball dal VPS
```bash
# sul VPS
./scripts/pack-src.sh ~/izanagi-src.tgz
# sul Mac
scp USER@HOST:~/izanagi-src.tgz ~/Downloads/
mkdir -p ~/src/new-izanagi && tar -xzf ~/Downloads/izanagi-src.tgz -C ~/src/new-izanagi
cd ~/src/new-izanagi && brew install qt cmake && ./scripts/build.sh
```

### Avvio
```bash
open ~/src/new-izanagi/build/app/izanagi.app
```

## Windows (MSVC + Qt)
1. Installa Qt 6 e Visual Studio
2. Apri "x64 Native Tools" e:
```bat
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.x\msvcxxxx_64
cmake --build build --config Release
```
Eseguibile: `build\app\Release\izanagi.exe`
