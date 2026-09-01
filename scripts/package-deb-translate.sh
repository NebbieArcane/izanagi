#!/usr/bin/env bash
# Build a Debian package (.deb) for Nebbie Translate (room descriptions only).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
DIST="${ROOT}/dist"
STAGING=""
VERSION=""
ARCH="$(dpkg --print-architecture 2>/dev/null || echo amd64)"
PREFIX="/usr"
RUN_BUILD=1

usage() {
    cat <<'EOF'
Usage: ./scripts/package-deb-translate.sh [options]

Builds dist/cypher_<version>_<arch>.deb

Options:
  --no-build       Skip ./scripts/build.sh --no-qt (translator only)
  --prefix PATH    Install prefix inside the package (default: /usr)
  -h, --help       Show this help
EOF
}

cleanup() {
    if [[ -n "${STAGING}" && -d "${STAGING}" ]]; then
        rm -rf "${STAGING}"
    fi
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
        --prefix) PREFIX="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "ERROR: package-deb-translate.sh must run on Linux." >&2
    exit 1
fi

if ! command -v dpkg-deb >/dev/null 2>&1; then
    echo "ERROR: dpkg-deb not found (install dpkg)." >&2
    exit 1
fi

trap cleanup EXIT
read_version

if [[ "${RUN_BUILD}" -eq 1 ]]; then
    "${ROOT}/scripts/build.sh" --no-qt
fi

echo "==> Preparing bundled sample lib (getworldlocal)"
"${ROOT}/scripts/prepare-sample-lib.sh"

if [[ ! -x "${BUILD}/nebbie-translator/cypher" && ! -x "${BUILD}/nebbie-translator/nebbie-translate" ]]; then
    echo "ERROR: cypher binary missing. Build first: ./scripts/build.sh --no-qt" >&2
    exit 1
fi

STAGING="$(mktemp -d)"
mkdir -p "${DIST}"

echo "==> Installing cypher into package staging (${PREFIX})"
DESTDIR="${STAGING}" cmake --install "${BUILD}" --prefix "${PREFIX}"

mkdir -p "${STAGING}/usr/share/nebbie-translate"
cp -a "${ROOT}/dist/sample-mudroot" "${STAGING}/usr/share/nebbie-translate/"

mkdir -p "${STAGING}/DEBIAN"
INSTALLED_SIZE="$(du -sk "${STAGING}" | awk '{print $1}')"

cat > "${STAGING}/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
fi
EOF
chmod 755 "${STAGING}/DEBIAN/postinst"

cat > "${STAGING}/DEBIAN/control" <<EOF
Package: nebbie-translate
Version: ${VERSION}
Section: editors
Priority: optional
Architecture: ${ARCH}
Depends: libc6 (>= 2.31), libstdc++6 (>= 10), libqt6core6 (>= 6.2.0) | libqt6core6t64 (>= 6.2.0), libqt6gui6 (>= 6.2.0) | libqt6gui6t64 (>= 6.2.0), libqt6widgets6 (>= 6.2.0) | libqt6widgets6t64 (>= 6.2.0)
Maintainer: Nebbie Editor <nebbie-editor@local>
Installed-Size: ${INSTALLED_SIZE}
Description: Lightweight room translator for Nebbie Arcane MUD
 Nebbie Translate (Cypher) edits room names, descriptions, extra descriptions and
 exit look text in myst.wld for translation workflows.
 Includes sample mudroot/lib under /usr/share/nebbie-translate/sample-mudroot.
EOF

DEB_FILE="${DIST}/cypher_${VERSION}_${ARCH}.deb"
echo "==> Building ${DEB_FILE}"
dpkg-deb --root-owner-group --build "${STAGING}" "${DEB_FILE}"

echo ""
echo "Package created:"
echo "  ${DEB_FILE}"
