#!/usr/bin/env bash
# Mirror of NebbieArcane/Server/getworldlocal: copy myst.* into mudroot/lib.
#
# Usage:
#   ./scripts/getworldlocal.sh [SERVER_ROOT]
#
# SERVER_ROOT defaults to vendor/nebbietest (clone via fetch-test-data.sh).
# Expects myst.zon, myst.wld, myst.mob, myst.obj, ... at SERVER_ROOT root
# or already under SERVER_ROOT/mudroot/lib (no-op if lib already has them).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SERVER_ROOT="${1:-${ROOT}/vendor/nebbietest}"

if [[ ! -d "${SERVER_ROOT}" ]]; then
    echo "ERROR: server root not found: ${SERVER_ROOT}" >&2
    echo "Run: ./scripts/fetch-test-data.sh" >&2
    exit 1
fi

LIB="${SERVER_ROOT}/mudroot/lib"
mkdir -p "${LIB}"

shopt -s nullglob
MYST_SRC=("${SERVER_ROOT}"/myst.*)
MYST_LIB=("${LIB}"/myst.*)

# Working tree sometimes loses myst.* after rsync/partial copy while .git stays.
# If this is a git checkout, restore tracked myst.* from HEAD.
if [[ ${#MYST_SRC[@]} -eq 0 && -d "${SERVER_ROOT}/.git" ]]; then
    echo "==> No myst.* in ${SERVER_ROOT}; trying git restore from HEAD"
    # Portable (macOS /bin/bash 3.2 has no mapfile)
    while IFS= read -r tracked; do
        [[ -n "${tracked}" ]] || continue
        git -C "${SERVER_ROOT}" checkout HEAD -- "${tracked}"
    done < <(git -C "${SERVER_ROOT}" ls-files 'myst.*' 2>/dev/null || true)
    MYST_SRC=("${SERVER_ROOT}"/myst.*)
fi

if [[ ${#MYST_SRC[@]} -eq 0 ]]; then
    if [[ ${#MYST_LIB[@]} -gt 0 ]]; then
        echo "==> getworldlocal: myst.* already present in ${LIB} (skip copy from root)"
        for subdir in objects mobiles rooms zones; do
            mkdir -p "${LIB}/${subdir}"
        done
        echo "Sample lib ready: ${LIB}"
        exit 0
    fi
    echo "ERROR: no myst.* files in ${SERVER_ROOT} (nor in ${LIB})" >&2
    echo "Tip: rm -rf \"${SERVER_ROOT}\" && ./scripts/fetch-test-data.sh" >&2
    exit 1
fi

echo "==> getworldlocal: ${SERVER_ROOT} -> ${LIB}"
for src in "${MYST_SRC[@]}"; do
    base="$(basename "${src}")"
    # Skip runtime pid file if present at repo root.
    if [[ "${base}" == "myst.pid" ]]; then
        continue
    fi
    cp -v "${src}" "${LIB}/${base}"
done

for subdir in objects mobiles rooms zones; do
    mkdir -p "${LIB}/${subdir}"
done

echo "Sample lib ready: ${LIB}"
