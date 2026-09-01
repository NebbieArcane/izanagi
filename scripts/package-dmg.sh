#!/usr/bin/env bash
# Build a macOS disk image (.dmg) with Izanagi.app and nebbiedit CLI.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
DIST="${ROOT}/dist"
STAGING="${ROOT}/dist/dmg-staging"
VERSION=""
RUN_BUILD=1

usage() {
    cat <<'EOF'
Usage: ./scripts/package-dmg.sh [options]

Builds dist/izanagi_<version>_macos.dmg containing:
  - Izanagi.app
  - bin/nebbiedit (CLI)
  - Applications symlink (drag-and-drop install)

Options:
  --no-build       Skip ./scripts/build.sh --macos-bundle
  -h, --help       Show this help

Requires: macOS with hdiutil and a Qt 6 build (--macos-bundle).
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
    echo "ERROR: package-dmg.sh must run on macOS (needs hdiutil)." >&2
    exit 1
fi

if ! command -v hdiutil >/dev/null 2>&1; then
    echo "ERROR: hdiutil not found." >&2
    exit 1
fi

read_version
mkdir -p "${DIST}"

if [[ ! -f "${ROOT}/nebbie-qt/icons/izanagi.icns" && ! -f "${ROOT}/nebbie-qt/icons/nebbieedit.icns" ]]; then
    echo "==> Generating app icons"
    python3 "${ROOT}/scripts/generate-nebbie-icons.py" nebbieedit
fi

if [[ "${RUN_BUILD}" -eq 1 ]]; then
    export CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-$(brew --prefix qt@6 2>/dev/null || true)}"
    "${ROOT}/scripts/build.sh" --macos-bundle
fi

echo "==> Preparing bundled sample lib (getworldlocal)"
"${ROOT}/scripts/prepare-sample-lib.sh"

APP_SRC="${BUILD}/nebbie-qt/Izanagi.app"
if [[ ! -d "${APP_SRC}" ]]; then
    APP_SRC="${BUILD}/nebbie-qt/nebbieedit.app"
fi
CLI_SRC="${BUILD}/nebbiedit/nebbiedit"

if [[ ! -d "${APP_SRC}" ]]; then
    echo "ERROR: Izanagi.app not found under ${BUILD}/nebbie-qt/. Run: ./scripts/build.sh --macos-bundle" >&2
    exit 1
fi
if [[ ! -x "${CLI_SRC}" ]]; then
    echo "ERROR: ${CLI_SRC} not found." >&2
    exit 1
fi

echo "==> Bundling Qt and signing app"
ENTITLEMENTS="${ROOT}/nebbie-qt/macos/entitlements.plist"
if [[ ! -f "${ENTITLEMENTS}" ]]; then
    ENTITLEMENTS="${ROOT}/nebbie-translator/macos/entitlements.plist"
fi
"${ROOT}/scripts/macos-prepare-app-bundle.sh" "${APP_SRC}" "${ENTITLEMENTS}"

echo "==> Preparing DMG staging"
rm -rf "${STAGING}"
mkdir -p "${STAGING}/bin"
cp -R "${APP_SRC}" "${STAGING}/"
cp "${CLI_SRC}" "${STAGING}/bin/"
cp -a "${DIST}/sample-mudroot" "${STAGING}/"
cat > "${STAGING}/LEGGIMI.txt" <<'EOF'
Izanagi
=======

1. Trascina Izanagi.app nella cartella Applicazioni
2. Avvia Izanagi → File → Apri libreria → mudroot o mudroot/lib

Se macOS blocca l'app al primo avvio: tasto destro sull'app → Apri,
oppure in Terminale: xattr -cr /Applications/Izanagi.app

CLI incluso: bin/nebbiedit
Mondo di prova: sample-mudroot/lib
EOF
ln -sf /Applications "${STAGING}/Applications"

DMG_FILE="${DIST}/izanagi_${VERSION}_macos.dmg"
rm -f "${DMG_FILE}"

echo "==> Creating ${DMG_FILE}"
hdiutil create \
    -volname "Izanagi" \
    -srcfolder "${STAGING}" \
    -ov \
    -format UDZO \
    "${DMG_FILE}"

rm -rf "${STAGING}"

echo ""
echo "Disk image created:"
echo "  ${DMG_FILE}"
ls -lh "${DMG_FILE}"
echo ""
echo "Users can drag Izanagi.app to Applications."
