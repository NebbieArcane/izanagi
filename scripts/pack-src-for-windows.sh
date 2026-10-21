#!/usr/bin/env bash
# Create a source tarball suitable for building Izanagi on a Windows PC
# (no GitHub push). Run on the NUC from the izanagi repo root or any cwd.
#
#   ./scripts/pack-src-for-windows.sh
#   ./scripts/pack-src-for-windows.sh /tmp/izanagi-wip.tgz
#
# Excludes build/ and heavy dist packages; keeps .git so the Windows tree
# stays a normal clone if you already had one.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-${ROOT}/dist/izanagi-wip-src.tgz}"
mkdir -p "$(dirname "$OUT")"

echo "==> packing ${ROOT} -> ${OUT}"
tar -C "$(dirname "$ROOT")" \
    --exclude='izanagi/build' \
    --exclude='izanagi/dist/*.zip' \
    --exclude='izanagi/dist/*.exe' \
    --exclude='izanagi/dist/*.dmg' \
    --exclude='izanagi/dist/*.deb' \
    --exclude='izanagi/dist/*.tgz' \
    --exclude='izanagi/.cache' \
    -czf "${OUT}" "$(basename "$ROOT")"

BYTES="$(wc -c < "${OUT}" | tr -d ' ')"
echo "OK: ${OUT} (${BYTES} bytes)"
echo "On Windows (PowerShell), from a folder that contains scripts\\:"
echo "  .\\scripts\\pull-nuc-and-build.ps1 -NucHost nebbie@IP_NUC -FetchOnly"
echo "  # or all-in-one:"
echo "  .\\scripts\\pull-nuc-and-build.ps1 -NucHost nebbie@IP_NUC"
