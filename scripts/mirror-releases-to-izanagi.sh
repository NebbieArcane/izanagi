#!/usr/bin/env bash
# Copia le release izanagi/cypher dal fork wizardmorgan su NebbieArcane/izanagi.
# Esegui in locale con: gh auth login (account con write su NebbieArcane/izanagi)
set -euo pipefail

SRC_REPO="${SRC_REPO:-wizardmorgan/nebbie-arcane-editing-tools}"
DST_REPO="${DST_REPO:-NebbieArcane/izanagi}"

tmpdir="$(mktemp -d)"
cleanup() { rm -rf "$tmpdir"; }
trap cleanup EXIT

mirror_tag() {
  local tag="$1"
  local title="$2"
  local dir="$tmpdir/$tag"

  mkdir -p "$dir"
  echo "==> Download $tag da $SRC_REPO"
  gh release download "$tag" --repo "$SRC_REPO" --dir "$dir"

  if gh release view "$tag" --repo "$DST_REPO" >/dev/null 2>&1; then
    echo "==> Upload asset su release esistente $tag ($DST_REPO)"
    gh release upload "$tag" --repo "$DST_REPO" --clobber "$dir"/*
  else
    echo "==> Crea release $tag su $DST_REPO"
    gh release create "$tag" --repo "$DST_REPO" --title "$title" --prerelease \
      --target main --notes "Release $title migrata su $DST_REPO" "$dir"/*
  fi
}

mirror_tag izanagi Izanagi
mirror_tag cypher Cypher

echo "OK: https://github.com/$DST_REPO/releases"
