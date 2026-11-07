#!/usr/bin/env bash
# Pacchetto Linux portable (.tar.gz) con binario + sample + catalogs.
# Dipende da Qt6 di sistema (libqt6*). Per uso tipico su Ubuntu/Debian recenti.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
DIST="${ROOT}/dist"
VERSION="${NEBBIE_VERSION:-0.1.0}"
RUN_BUILD=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-build) RUN_BUILD=0; shift ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "ERROR: package-linux.sh va eseguito su Linux." >&2
  exit 1
fi

if [[ "${RUN_BUILD}" -eq 1 ]]; then
  "${ROOT}/scripts/build.sh"
fi

BIN="${BUILD}/app/eleutheria"
if [[ ! -x "${BIN}" ]]; then
  echo "ERROR: manca ${BIN}" >&2
  exit 1
fi

STAGING="${DIST}/linux-staging"
rm -rf "${STAGING}"
mkdir -p "${STAGING}/bin" "${STAGING}/share/eleutheria" "${DIST}"

cp -a "${BIN}" "${STAGING}/bin/eleutheria"
cp -a "${ROOT}/sample" "${STAGING}/share/eleutheria/"
cp -a "${ROOT}/catalogs" "${STAGING}/share/eleutheria/"
cp -a "${ROOT}/README.md" "${STAGING}/README.md"

cat > "${STAGING}/LEGGIMI.txt" <<EOF
Eleutheria / Izanagi (restyle) — Linux
======================================

1. Estrai l'archivio
2. Installa Qt6 runtime se manca, es. su Ubuntu/Debian:
     sudo apt install libqt6widgets6 libqt6gui6 libqt6core6
3. Avvia:
     ./bin/eleutheria

Mondo di esempio: share/eleutheria/sample/B1
Versione pacchetto: ${VERSION}
EOF

ARCH="$(uname -m)"
OUT="${DIST}/eleutheria_${VERSION}_linux_${ARCH}.tar.gz"
tar -C "${STAGING}" -czf "${OUT}" .
echo "Creato: ${OUT}"
ls -la "${OUT}"
