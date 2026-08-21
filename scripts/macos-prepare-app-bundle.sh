#!/usr/bin/env bash
# Bundle Qt frameworks into a .app and ad-hoc sign it for macOS distribution.
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 /path/to/App.app" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_PATH="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
ENTITLEMENTS="${2:-${ROOT}/nebbie-translator/macos/entitlements.plist}"
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

sign_bundle_adhoc() {
    local target="$1"
    if [[ -d "${target}/Contents/Frameworks" ]]; then
        find "${target}/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.framework" \) -print0 2>/dev/null \
            | while IFS= read -r -d '' lib; do
                codesign --force --sign - --timestamp=none "${lib}" 2>/dev/null || true
            done
    fi
    if [[ -d "${target}/Contents/PlugIns" ]]; then
        find "${target}/Contents/PlugIns" -type f -name "*.dylib" -print0 2>/dev/null \
            | while IFS= read -r -d '' plugin; do
                codesign --force --sign - --timestamp=none "${plugin}" 2>/dev/null || true
            done
    fi
    if [[ -f "${target}/Contents/MacOS/"* ]]; then
        for bin in "${target}/Contents/MacOS/"*; do
            [[ -f "${bin}" ]] || continue
            codesign --force --sign - --timestamp=none "${bin}"
        done
    fi
    codesign --force --sign - --timestamp=none "${target}"
}

MACDEPLOYQT="$(find_macdeployqt || true)"
if [[ -z "${MACDEPLOYQT}" ]]; then
    echo "ERROR: macdeployqt not found (install Qt 6 or set CMAKE_PREFIX_PATH)" >&2
    exit 1
fi

echo "==> macdeployqt $(basename "${APP_PATH}")"
if ! "${MACDEPLOYQT}" "${APP_PATH}"; then
    echo "ERROR: macdeployqt failed for ${APP_PATH}" >&2
    exit 1
fi

should_notarize_app() {
    [[ -n "${APPLE_TEAM_ID:-}" ]] || return 1
    if [[ -n "${APP_STORE_CONNECT_API_KEY_ID:-}" && -n "${APP_STORE_CONNECT_API_ISSUER_ID:-}" ]]; then
        [[ -n "${APP_STORE_CONNECT_API_KEY_PATH:-}" || -n "${APP_STORE_CONNECT_API_KEY_BASE64:-}" ]]
        return
    fi
    [[ -n "${APPLE_ID:-}" && -n "${APPLE_APP_SPECIFIC_PASSWORD:-}" ]]
}

if should_notarize_app && security find-identity -v -p codesigning 2>/dev/null | grep -q 'Developer ID Application'; then
    echo "==> Developer ID sign + notarization"
    "${ROOT}/scripts/macos-notarize-app.sh" "${APP_PATH}" "${ENTITLEMENTS}"
else
    echo "==> Ad-hoc codesign (bundle + embedded Qt)"
    sign_bundle_adhoc "${APP_PATH}"
    if [[ "$(uname -s)" == "Darwin" ]] && should_notarize_app; then
        echo "WARNING: notarization skipped — Developer ID certificate not found in keychain." >&2
    fi
fi

if [[ ! -d "${APP_PATH}/Contents/Frameworks" ]]; then
    echo "ERROR: macdeployqt did not create Contents/Frameworks" >&2
    exit 1
fi

echo "==> App bundle ready: ${APP_PATH}"
