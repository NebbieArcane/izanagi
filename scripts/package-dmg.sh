#!/usr/bin/env bash
# Build a macOS disk image (.dmg) with a self-contained Izanagi.app.
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
  - Izanagi.app (GUI + CLI + sample-mudroot inside the bundle)
  - Applications symlink (drag-and-drop install)
  - LEGGIMI.txt (instructions only; no need to copy)

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

echo "==> Embedding sample lib and CLI into app bundle"
"${ROOT}/scripts/macos-embed-app-extras.sh" "${APP_SRC}" --cli "${CLI_SRC}"

echo "==> Bundling Qt and signing app"
ENTITLEMENTS="${ROOT}/nebbie-qt/macos/entitlements.plist"
if [[ ! -f "${ENTITLEMENTS}" ]]; then
    ENTITLEMENTS="${ROOT}/nebbie-translator/macos/entitlements.plist"
fi
"${ROOT}/scripts/macos-prepare-app-bundle.sh" "${APP_SRC}" "${ENTITLEMENTS}"

echo "==> Preparing DMG staging"
rm -rf "${STAGING}"
mkdir -p "${STAGING}"
cp -R "${APP_SRC}" "${STAGING}/"
cat > "${STAGING}/LEGGIMI.txt" <<'EOF'
Izanagi
=======

Installazione (un solo passaggio):
1. Apri questo .dmg
2. Trascina Izanagi.app su Applicazioni
3. Avvia Izanagi

Dentro l'app trovi gia' tutto:
  - mondo di prova: File → Apri mondo di esempio
    (oppure Izanagi.app/Contents/Resources/sample-mudroot/lib)
  - CLI: /Applications/Izanagi.app/Contents/MacOS/nebbiedit

Se macOS dice che l'app e' "danneggiata" o non si apre:
  - NON e' il download corrotto: e' Gatekeeper su app non notarizzate.
  - Tasto destro su Izanagi.app → Apri (solo la prima volta)
  - oppure in Terminale:
      xattr -cr /Applications/Izanagi.app
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

if ! hdiutil verify "${DMG_FILE}" >/dev/null; then
    echo "ERROR: DMG verification failed for ${DMG_FILE}" >&2
    exit 1
fi
echo "==> DMG verification passed"

rm -rf "${STAGING}"

echo ""
echo "Disk image created:"
echo "  ${DMG_FILE}"
ls -lh "${DMG_FILE}"
echo ""
echo "Users only need to drag Izanagi.app to Applications."
