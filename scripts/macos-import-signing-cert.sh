#!/usr/bin/env bash
# Import Apple code-signing certificate into a temporary keychain (CI/local).
set -euo pipefail

if [[ -z "${APPLE_CERTIFICATE_BASE64:-}" || -z "${APPLE_CERTIFICATE_PASSWORD:-}" ]]; then
    echo "ERROR: APPLE_CERTIFICATE_BASE64 and APPLE_CERTIFICATE_PASSWORD must be set." >&2
    exit 1
fi

KEYCHAIN_PASSWORD="${KEYCHAIN_PASSWORD:-$(openssl rand -hex 16)}"
CERTIFICATE_PATH="${RUNNER_TEMP:-/tmp}/build_certificate.p12"
KEYCHAIN_PATH="${RUNNER_TEMP:-/tmp}/app-signing.keychain-db"

echo -n "${APPLE_CERTIFICATE_BASE64}" | base64 --decode > "${CERTIFICATE_PATH}"

security create-keychain -p "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_PATH}"
security set-keychain-settings -lut 21600 "${KEYCHAIN_PATH}"
security unlock-keychain -p "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_PATH}"
security import "${CERTIFICATE_PATH}" \
    -P "${APPLE_CERTIFICATE_PASSWORD}" \
    -A \
    -t cert \
    -f pkcs12 \
    -k "${KEYCHAIN_PATH}"
security list-keychain -d user -s "${KEYCHAIN_PATH}"
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_PATH}"

echo "==> Signing identities available:"
security find-identity -v -p codesigning "${KEYCHAIN_PATH}"
