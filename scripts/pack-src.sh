#!/usr/bin/env bash
# Sul VPS: crea un tarball leggero da scaricare sul Mac (alternativa a rsync).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$HOME/eleutheria-src.tgz}"
tar -C "${ROOT}" -czf "${OUT}" \
  --exclude build \
  --exclude eleutheria-src \
  --exclude '.git' \
  CMakeLists.txt README.md .gitignore \
  app core catalogs docs sample scripts tests
echo "Creato: ${OUT}"
echo "Sul Mac:"
echo "  scp USER@HOST:${OUT} ~/Downloads/"
echo "  mkdir -p ~/src/new-izanagi && tar -xzf ~/Downloads/$(basename "${OUT}") -C ~/src/new-izanagi"
echo "  cd ~/src/new-izanagi && ./scripts/build.sh"
