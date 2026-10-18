#!/usr/bin/env bash
# Build a macOS disk image (.dmg) with Cypher.app only.
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

Builds dist/cypher_<version>_macos.dmg containing Cypher.app

Options:
  --no-build       Skip ./scripts/build.sh --no-qt --macos-bundle-translator
  -h, --help       Show this help
EOF
}

read_version() {
    # shellcheck disable=SC1091
    source "${ROOT}/scripts/nebbie-version.sh"
    VERSION="$(nebbie_resolve_version "${BUILD}" "${ROOT}")"
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

APP_SRC="${BUILD}/nebbie-translator/Cypher.app"
if [[ ! -d "${APP_SRC}" ]]; then
    APP_SRC="${BUILD}/nebbie-translator/nebbie-translate.app"
fi
if [[ ! -d "${APP_SRC}" ]]; then
    echo "ERROR: Cypher.app not found under ${BUILD}/nebbie-translator. Run: ./scripts/build.sh --no-qt --macos-bundle-translator" >&2
    exit 1
fi

echo "==> Embedding sample lib into app bundle"
"${ROOT}/scripts/macos-embed-app-extras.sh" "${APP_SRC}"

echo "==> Bundling Qt and signing app"
"${ROOT}/scripts/macos-prepare-app-bundle.sh" "${APP_SRC}"

echo "==> Preparing DMG staging"
rm -rf "${STAGING}"
mkdir -p "${STAGING}"
cp -R "${APP_SRC}" "${STAGING}/"
cat > "${STAGING}/LEGGIMI.txt" <<'EOF'
Cypher (macOS)
==============

Installazione (un solo passaggio):
1. Apri questo .dmg
2. Trascina Cypher.app su Applicazioni
3. Avvia Cypher

Mondo di prova dentro l'app:
  Cypher.app/Contents/Resources/sample-mudroot/lib

Se macOS dice che l'app e' "danneggiata" o non si apre:
  - NON e' il download corrotto: e' Gatekeeper su app non notarizzate.
  - Tasto destro su Cypher.app → Apri (solo la prima volta)
  - oppure in Terminale:
      xattr -cr /Applications/Cypher.app
EOF
ln -sf /Applications "${STAGING}/Applications"

DMG_FILE="${DIST}/cypher_${VERSION}_macos.dmg"
rm -f "${DMG_FILE}"

echo "==> Creating ${DMG_FILE}"
hdiutil create \
    -volname "Cypher" \
    -srcfolder "${STAGING}" \
    -ov \
    -format UDZO \
    "${DMG_FILE}"

if ! hdiutil verify "${DMG_FILE}" >/dev/null; then
    echo "ERROR: DMG verification failed for ${DMG_FILE}" >&2
    exit 1
fi
echo "==> DMG verification passed"

rm -rf "${STAGING}"

echo ""
echo "Disk image created:"
echo "  ${DMG_FILE}"
