#!/usr/bin/env bash
# Embed sample-mudroot (and optional CLI) into a macOS .app before codesign.
# Call this before macos-prepare-app-bundle.sh so signing covers the extras.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${ROOT}/dist"

usage() {
    cat <<'EOF'
Usage: ./scripts/macos-embed-app-extras.sh /path/to/App.app [--cli /path/to/nebbiedit]

Copies dist/sample-mudroot into App.app/Contents/Resources/sample-mudroot.
With --cli, also installs the binary as Contents/MacOS/nebbiedit.
EOF
}

if [[ $# -lt 1 || "$1" == "-h" || "$1" == "--help" ]]; then
    usage
    exit 0
fi

APP_PATH="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
shift

CLI_SRC=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cli)
            CLI_SRC="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ ! -d "${APP_PATH}/Contents" ]]; then
    echo "ERROR: not an app bundle: ${APP_PATH}" >&2
    exit 1
fi

SAMPLE_SRC="${DIST}/sample-mudroot"
if [[ ! -d "${SAMPLE_SRC}/lib" ]]; then
    echo "ERROR: ${SAMPLE_SRC}/lib missing. Run ./scripts/prepare-sample-lib.sh first." >&2
    exit 1
fi

RESOURCES="${APP_PATH}/Contents/Resources"
MACOS_DIR="${APP_PATH}/Contents/MacOS"
mkdir -p "${RESOURCES}" "${MACOS_DIR}"

echo "==> Embedding sample-mudroot into $(basename "${APP_PATH}")"
rm -rf "${RESOURCES}/sample-mudroot"
cp -a "${SAMPLE_SRC}" "${RESOURCES}/"

if [[ -n "${CLI_SRC}" ]]; then
    if [[ ! -x "${CLI_SRC}" ]]; then
        echo "ERROR: CLI not found or not executable: ${CLI_SRC}" >&2
        exit 1
    fi
    echo "==> Embedding CLI as Contents/MacOS/nebbiedit"
    cp "${CLI_SRC}" "${MACOS_DIR}/nebbiedit"
    chmod +x "${MACOS_DIR}/nebbiedit"
fi

echo "==> Bundle extras ready under ${APP_PATH}/Contents"
