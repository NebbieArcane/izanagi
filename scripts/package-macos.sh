#!/usr/bin/env bash
# Pacchetto macOS .dmg con eleutheria.app + Qt bundlato via macdeployqt.
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

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: package-macos.sh va eseguito su macOS." >&2
  exit 1
fi

export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-$(brew --prefix qt@6 2>/dev/null || true)}"

if [[ "${RUN_BUILD}" -eq 1 ]]; then
  "${ROOT}/scripts/build.sh"
fi

APP="${BUILD}/app/eleutheria.app"
if [[ ! -d "${APP}" ]]; then
  echo "ERROR: manca ${APP}" >&2
  exit 1
fi

# Bundla Qt dentro l'app
MACDEPLOYQT=""
if [[ -n "${CMAKE_PREFIX_PATH}" && -x "${CMAKE_PREFIX_PATH}/bin/macdeployqt" ]]; then
  MACDEPLOYQT="${CMAKE_PREFIX_PATH}/bin/macdeployqt"
elif command -v macdeployqt >/dev/null 2>&1; then
  MACDEPLOYQT="$(command -v macdeployqt)"
fi
if [[ -z "${MACDEPLOYQT}" ]]; then
  echo "ERROR: macdeployqt non trovato" >&2
  exit 1
fi

# Copia sample nel bundle
mkdir -p "${APP}/Contents/Resources/sample"
cp -a "${ROOT}/sample/." "${APP}/Contents/Resources/sample/"
cp -a "${ROOT}/catalogs" "${APP}/Contents/Resources/" 2>/dev/null || true

"${MACDEPLOYQT}" "${APP}" -always-overwrite

STAGING="${DIST}/dmg-staging"
rm -rf "${STAGING}"
mkdir -p "${STAGING}" "${DIST}"
cp -R "${APP}" "${STAGING}/"
ln -s /Applications "${STAGING}/Applications"
cat > "${STAGING}/LEGGIMI.txt" <<EOF
Eleutheria / Izanagi (restyle) — macOS
======================================

Trascina eleutheria.app in Applicazioni, poi avviala.

Mondo di esempio incluso in Contents/Resources/sample/B1
Versione: ${VERSION}
EOF

OUT="${DIST}/eleutheria_${VERSION}_macos.dmg"
rm -f "${OUT}"
hdiutil create -volname "Eleutheria" -srcfolder "${STAGING}" -ov -format UDZO "${OUT}"
echo "Creato: ${OUT}"
ls -la "${OUT}"
