#!/usr/bin/env bash
# Bundle Qt frameworks into a .app and ad-hoc sign it for macOS distribution.
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 /path/to/App.app" >&2
    exit 1
fi

APP_PATH="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
if [[ ! -d "${APP_PATH}" ]]; then
    echo "ERROR: not an app bundle: ${APP_PATH}" >&2
    exit 1
fi

find_macdeployqt() {
    if command -v macdeployqt >/dev/null 2>&1; then
        command -v macdeployqt
        return 0
    fi
    local qt_prefix="${CMAKE_PREFIX_PATH:-}"
    if [[ -z "${qt_prefix}" ]] && command -v brew >/dev/null 2>&1; then
        qt_prefix="$(brew --prefix qt@6 2>/dev/null || true)"
    fi
    if [[ -n "${qt_prefix}" && -x "${qt_prefix}/bin/macdeployqt" ]]; then
        echo "${qt_prefix}/bin/macdeployqt"
        return 0
    fi
    return 1
}

MACDEPLOYQT="$(find_macdeployqt || true)"
if [[ -z "${MACDEPLOYQT}" ]]; then
    echo "ERROR: macdeployqt not found (install Qt 6 or set CMAKE_PREFIX_PATH)" >&2
    exit 1
fi

echo "==> macdeployqt $(basename "${APP_PATH}")"
"${MACDEPLOYQT}" "${APP_PATH}" -always-run-macdeployqt -codesign=-

if ! codesign --verify --verbose=2 "${APP_PATH}" 2>/dev/null; then
    echo "==> Ad-hoc codesign fallback"
    codesign --force --deep --sign - --timestamp=none "${APP_PATH}"
    codesign --verify --verbose=2 "${APP_PATH}"
fi

echo "==> App bundle ready: ${APP_PATH}"
