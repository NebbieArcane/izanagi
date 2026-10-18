#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TARGET="$ROOT/vendor/nebbietest"

has_root_myst() {
  [[ -f "${TARGET}/myst.zon" && -f "${TARGET}/myst.wld" && -f "${TARGET}/myst.obj" ]]
}

if [[ -d "$TARGET/.git" ]]; then
  git -C "$TARGET" pull --ff-only origin develop
  # Partial sync (rsync/transfer) can leave .git intact but delete working-tree myst.*.
  if ! has_root_myst; then
    echo "WARN: myst.* missing after pull; restoring tracked files from HEAD" >&2
    # Portable (macOS /bin/bash 3.2 has no mapfile): restore each tracked myst.*
    while IFS= read -r tracked; do
      [[ -n "${tracked}" ]] || continue
      git -C "$TARGET" checkout HEAD -- "${tracked}"
    done < <(git -C "$TARGET" ls-files 'myst.*' 2>/dev/null || true)
  fi
  if ! has_root_myst; then
    echo "WARN: myst.* still missing; recloning vendor/nebbietest" >&2
    rm -rf "$TARGET"
    mkdir -p "$ROOT/vendor"
    git clone --depth 1 --branch develop https://github.com/wizardmorgan/nebbietest.git "$TARGET"
  fi
else
  mkdir -p "$ROOT/vendor"
  git clone --depth 1 --branch develop https://github.com/wizardmorgan/nebbietest.git "$TARGET"
fi

if ! has_root_myst; then
  echo "ERROR: vendor/nebbietest has no myst.zon/wld/obj after fetch" >&2
  echo "Check https://github.com/wizardmorgan/nebbietest (branch develop)" >&2
  exit 1
fi

echo "Test data ready at $TARGET/mudroot/lib"

LIB="$TARGET/mudroot/lib"
mkdir -p "$LIB"
for file in myst.zon myst.wld myst.mob myst.obj myst.shp myst.spe; do
  if [[ -f "$TARGET/$file" && ! -f "$LIB/$file" ]]; then
    cp "$TARGET/$file" "$LIB/$file"
    echo "Installed $file -> mudroot/lib/"
  fi
done

"$ROOT/scripts/getworldlocal.sh" "$TARGET"
