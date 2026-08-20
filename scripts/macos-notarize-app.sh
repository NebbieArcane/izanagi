#!/usr/bin/env bash
# Sign a .app with Developer ID and notarize it with Apple (notarytool).
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 /path/to/App.app [entitlements.plist]" >&2
    exit 1
fi

APP_PATH="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
ENTITLEMENTS="${2:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/macos/entitlements.plist}"

if [[ ! -d "${APP_PATH}" ]]; then
    echo "ERROR: not an app bundle: ${APP_PATH}" >&2
    exit 1
fi

if [[ ! -f "${ENTITLEMENTS}" ]]; then
    echo "ERROR: entitlements file not found: ${ENTITLEMENTS}" >&2
    exit 1
fi

resolve_signing_identity() {
    if [[ -n "${APPLE_SIGNING_IDENTITY:-}" ]]; then
        echo "${APPLE_SIGNING_IDENTITY}"
        return 0
    fi
    local identity
    identity="$(security find-identity -v -p codesigning 2>/dev/null \
        | grep 'Developer ID Application' \
        | head -1 \
        | sed -n 's/.*"\(Developer ID Application:.*\)"/\1/p' || true)"
    if [[ -z "${identity}" ]]; then
        return 1
    fi
    echo "${identity}"
}

has_notary_credentials() {
    [[ -n "${APPLE_TEAM_ID:-}" ]] || return 1
    if [[ -n "${APP_STORE_CONNECT_API_KEY_ID:-}" && -n "${APP_STORE_CONNECT_API_ISSUER_ID:-}" ]]; then
        [[ -n "${APP_STORE_CONNECT_API_KEY_PATH:-}" || -n "${APP_STORE_CONNECT_API_KEY_BASE64:-}" ]]
        return
    fi
    [[ -n "${APPLE_ID:-}" && -n "${APPLE_APP_SPECIFIC_PASSWORD:-}" ]]
}

sign_file() {
    local target="$1"
    local identity="$2"
    codesign --force --options runtime --timestamp \
        --entitlements "${ENTITLEMENTS}" \
        --sign "${identity}" \
        "${target}"
}

sign_app_bundle() {
    local identity="$1"

    if [[ -d "${APP_PATH}/Contents/Frameworks" ]]; then
        find "${APP_PATH}/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.framework" \) -print0 2>/dev/null \
            | while IFS= read -r -d '' lib; do
                sign_file "${lib}" "${identity}"
            done
    fi

    if [[ -d "${APP_PATH}/Contents/PlugIns" ]]; then
        find "${APP_PATH}/Contents/PlugIns" -type f -name "*.dylib" -print0 2>/dev/null \
            | while IFS= read -r -d '' plugin; do
                sign_file "${plugin}" "${identity}"
            done
    fi

    for bin in "${APP_PATH}/Contents/MacOS/"*; do
        [[ -f "${bin}" ]] || continue
        sign_file "${bin}" "${identity}"
    done

    sign_file "${APP_PATH}" "${identity}"
    codesign --verify --deep --strict --verbose=2 "${APP_PATH}"
}

prepare_notary_api_key() {
    if [[ -n "${APP_STORE_CONNECT_API_KEY_PATH:-}" && -f "${APP_STORE_CONNECT_API_KEY_PATH}" ]]; then
        return 0
    fi
    if [[ -z "${APP_STORE_CONNECT_API_KEY_BASE64:-}" ]]; then
        return 1
    fi
    APP_STORE_CONNECT_API_KEY_PATH="${RUNNER_TEMP:-/tmp}/AuthKey_${APP_STORE_CONNECT_API_KEY_ID}.p8"
    echo -n "${APP_STORE_CONNECT_API_KEY_BASE64}" | base64 --decode > "${APP_STORE_CONNECT_API_KEY_PATH}"
    chmod 600 "${APP_STORE_CONNECT_API_KEY_PATH}"
}

submit_for_notarization() {
    local zip_path="$1"
    if [[ -n "${APP_STORE_CONNECT_API_KEY_ID:-}" && -n "${APP_STORE_CONNECT_API_ISSUER_ID:-}" ]]; then
        prepare_notary_api_key
        xcrun notarytool submit "${zip_path}" \
            --key "${APP_STORE_CONNECT_API_KEY_PATH}" \
            --key-id "${APP_STORE_CONNECT_API_KEY_ID}" \
            --issuer "${APP_STORE_CONNECT_API_ISSUER_ID}" \
            --wait
        return 0
    fi

    xcrun notarytool submit "${zip_path}" \
        --apple-id "${APPLE_ID}" \
        --password "${APPLE_APP_SPECIFIC_PASSWORD}" \
        --team-id "${APPLE_TEAM_ID}" \
        --wait
}

notarize_app_bundle() {
    local zip_path
    zip_path="$(mktemp -t nebbie-notarize).zip"
    ditto -c -k --keepParent "${APP_PATH}" "${zip_path}"
    echo "==> Submitting $(basename "${APP_PATH}") to Apple notarization"
    submit_for_notarization "${zip_path}"
    rm -f "${zip_path}"
    echo "==> Stapling notarization ticket"
    xcrun stapler staple "${APP_PATH}"
    xcrun stapler validate "${APP_PATH}"
}

IDENTITY="$(resolve_signing_identity || true)"
if [[ -z "${IDENTITY}" ]]; then
    echo "ERROR: Developer ID Application certificate not found." >&2
    echo "Import signing certificate in keychain or set APPLE_SIGNING_IDENTITY." >&2
    exit 1
fi

if ! has_notary_credentials; then
    echo "ERROR: Apple notarization credentials missing." >&2
    echo "Set APPLE_TEAM_ID plus either:" >&2
    echo "  - APPLE_ID + APPLE_APP_SPECIFIC_PASSWORD" >&2
    echo "  - APP_STORE_CONNECT_API_KEY_ID + APP_STORE_CONNECT_API_ISSUER_ID + APP_STORE_CONNECT_API_KEY_BASE64" >&2
    exit 1
fi

echo "==> Developer ID sign: $(basename "${APP_PATH}")"
echo "    Identity: ${IDENTITY}"
sign_app_bundle "${IDENTITY}"
notarize_app_bundle "${APP_PATH}"
echo "==> Notarized app ready: ${APP_PATH}"
