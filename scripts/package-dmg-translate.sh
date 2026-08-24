#!/usr/bin/env bash
# Build a macOS disk image (.dmg) with nebbie-translate.app only.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
DIST="${ROOT}/dist"
STAGING="${ROOT}/dist/dmg-translate-staging"
VERSION=""
RUN_BUILD=1

usage() {
    cat <<'EOF'
Usage: ./scripts/package-dmg-translate.sh [options]

Builds dist/cypher_<version>_macos.dmg containing nebbie-translate.app

Options:
  --no-build       Skip ./scripts/build.sh --no-qt --macos-bundle-translator
  -h, --help       Show this help
EOF
}

read_version() {
    VERSION="$(sed -n 's/^project(nebbie-editor VERSION \([^ )]*\).*/\1/p' "${ROOT}/CMakeLists.txt")"
    if [[ -z "${VERSION}" ]]; then
        VERSION="0.0.0"
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-build) RUN_BUILD=0; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERROR: package-dmg-translate.sh must run on macOS." >&2
    exit 1
fi

read_version
mkdir -p "${DIST}"

if [[ "${RUN_BUILD}" -eq 1 ]]; then
    export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-$(brew --prefix qt@6 2>/dev/null || true)}"
    "${ROOT}/scripts/build.sh" --no-qt --macos-bundle-translator
fi

echo "==> Preparing bundled sample lib (getworldlocal)"
"${ROOT}/scripts/prepare-sample-lib.sh"

APP_SRC="${BUILD}/nebbie-translator/nebbie-translate.app"
if [[ ! -d "${APP_SRC}" ]]; then
    echo "ERROR: ${APP_SRC} not found. Run: ./scripts/build.sh --no-qt --macos-bundle-translator" >&2
    exit 1
fi

echo "==> Bundling Qt and signing app"
"${ROOT}/scripts/macos-prepare-app-bundle.sh" "${APP_SRC}"

echo "==> Preparing DMG staging"
rm -rf "${STAGING}"
mkdir -p "${STAGING}"
cp -R "${APP_SRC}" "${STAGING}/"
cp -a "${DIST}/sample-mudroot" "${STAGING}/"
cat > "${STAGING}/LEGGIMI.txt" <<'EOF'
Nebbie Translate (macOS)
======================

1. Trascina nebbie-translate.app nella cartella Applicazioni
2. Avvia Nebbie Translate → File → Apri libreria → mudroot o mudroot/lib

Se macOS blocca l'app al primo avvio: tasto destro sull'app → Apri,
oppure in Terminale: xattr -cr /Applications/nebbie-translate.app

Mondo di prova: sample-mudroot/lib
EOF
ln -sf /Applications "${STAGING}/Applications"

DMG_FILE="${DIST}/cypher_${VERSION}_macos.dmg"
rm -f "${DMG_FILE}"

echo "==> Creating ${DMG_FILE}"
hdiutil create \
    -volname "Nebbie Translate" \
    -srcfolder "${STAGING}" \
    -ov \
    -format UDZO \
    "${DMG_FILE}"

rm -rf "${STAGING}"

echo ""
echo "Disk image created:"
echo "  ${DMG_FILE}"
