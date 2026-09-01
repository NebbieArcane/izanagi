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

restore_bundle_icon() {
    local app_path="$1"
    local plist="${app_path}/Contents/Info.plist"
    local resources="${app_path}/Contents/Resources"
    [[ -f "${plist}" ]] || return 0

    local app_name
    app_name="$(basename "${app_path}" .app)"
    local icns_src=""
    local icns_name=""

    case "${app_name}" in
        Izanagi)
            icns_name="izanagi"
            if [[ -f "${ROOT}/nebbie-qt/icons/izanagi.icns" ]]; then
                icns_src="${ROOT}/nebbie-qt/icons/izanagi.icns"
            elif [[ -f "${ROOT}/nebbie-qt/icons/nebbieedit.icns" ]]; then
                icns_src="${ROOT}/nebbie-qt/icons/nebbieedit.icns"
                icns_name="nebbieedit"
            fi
            ;;
        nebbieedit)
            icns_name="izanagi"
            if [[ -f "${ROOT}/nebbie-qt/icons/izanagi.icns" ]]; then
                icns_src="${ROOT}/nebbie-qt/icons/izanagi.icns"
            elif [[ -f "${ROOT}/nebbie-qt/icons/nebbieedit.icns" ]]; then
                icns_src="${ROOT}/nebbie-qt/icons/nebbieedit.icns"
                icns_name="nebbieedit"
            fi
            ;;
        Cypher|nebbie-translate)
            icns_name="cypher"
            if [[ -f "${ROOT}/nebbie-translator/icons/cypher.icns" ]]; then
                icns_src="${ROOT}/nebbie-translator/icons/cypher.icns"
            elif [[ -f "${ROOT}/nebbie-translator/icons/nebbie-translate.icns" ]]; then
                icns_src="${ROOT}/nebbie-translator/icons/nebbie-translate.icns"
                icns_name="nebbie-translate"
            fi
            ;;
        *)
            return 0
            ;;
    esac

    if [[ -z "${icns_src}" ]]; then
        echo "WARNING: no .icns found for ${app_name}" >&2
        return 0
    fi

    cp "${icns_src}" "${resources}/${icns_name}.icns"
    /usr/libexec/PlistBuddy -c "Delete :CFBundleIconFile" "${plist}" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string ${icns_name}" "${plist}"
    echo "==> Bundle icon set to ${icns_name}.icns"
}

sign_bundle_adhoc() {
    local target="$1"

    if [[ -d "${target}/Contents/Frameworks" ]]; then
        while IFS= read -r -d '' lib; do
            codesign --force --sign - --timestamp=none "${lib}"
        done < <(find "${target}/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.so" \) -print0 2>/dev/null)

        while IFS= read -r -d '' framework; do
            codesign --force --sign - --timestamp=none "${framework}"
        done < <(find "${target}/Contents/Frameworks" -type d -name "*.framework" -print0 2>/dev/null)
    fi

    if [[ -d "${target}/Contents/PlugIns" ]]; then
        while IFS= read -r -d '' plugin; do
            codesign --force --sign - --timestamp=none "${plugin}"
        done < <(find "${target}/Contents/PlugIns" -type f -name "*.dylib" -print0 2>/dev/null)
    fi

    if [[ -d "${target}/Contents/MacOS" ]]; then
        for bin in "${target}/Contents/MacOS/"*; do
            [[ -f "${bin}" ]] || continue
            codesign --force --sign - --timestamp=none "${bin}"
        done
    fi

    codesign --force --sign - --timestamp=none "${target}"
}

verify_bundle_signature() {
    local target="$1"
    if ! codesign --verify --deep --strict --verbose=2 "${target}" 2>/dev/null; then
        echo "ERROR: codesign verification failed for ${target}" >&2
        codesign --verify --deep --strict --verbose=4 "${target}" >&2 || true
        return 1
    fi
    echo "==> codesign verification passed for $(basename "${target}")"
}

should_notarize_app() {
    [[ -n "${APPLE_TEAM_ID:-}" ]] || return 1
    if [[ -n "${APP_STORE_CONNECT_API_KEY_ID:-}" && -n "${APP_STORE_CONNECT_API_ISSUER_ID:-}" ]]; then
        [[ -n "${APP_STORE_CONNECT_API_KEY_PATH:-}" || -n "${APP_STORE_CONNECT_API_KEY_BASE64:-}" ]]
        return
    fi
    [[ -n "${APPLE_ID:-}" && -n "${APPLE_APP_SPECIFIC_PASSWORD:-}" ]]
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

if [[ ! -d "${APP_PATH}/Contents/Frameworks" ]]; then
    echo "ERROR: macdeployqt did not create Contents/Frameworks" >&2
    exit 1
fi

restore_bundle_icon "${APP_PATH}"

if should_notarize_app && security find-identity -v -p codesigning 2>/dev/null | grep -q 'Developer ID Application'; then
    echo "==> Developer ID sign + notarization"
    "${ROOT}/scripts/macos-notarize-app.sh" "${APP_PATH}" "${ENTITLEMENTS}"
else
    echo "==> Ad-hoc codesign (bundle + embedded Qt)"
    sign_bundle_adhoc "${APP_PATH}"
    verify_bundle_signature "${APP_PATH}"
    if [[ "$(uname -s)" == "Darwin" ]] && should_notarize_app; then
        echo "WARNING: notarization skipped — Developer ID certificate not found in keychain." >&2
    fi
fi

echo "==> App bundle ready: ${APP_PATH}"
