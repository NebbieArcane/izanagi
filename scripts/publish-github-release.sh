#!/usr/bin/env bash
# Create or refresh a GitHub release tag with flattened build artifacts.
# Deletes an existing release first so immutable-release repos can be updated.
set -euo pipefail

TAG="${1:?usage: publish-github-release.sh <tag> <title> <notes-file> <assets-dir>}"
TITLE="${2:?title required}"
NOTES_FILE="${3:?notes file required}"
ASSETS_DIR="${4:?assets directory required}"

if [[ ! -f "$NOTES_FILE" ]]; then
  echo "Notes file not found: $NOTES_FILE" >&2
  exit 1
fi

if [[ ! -d "$ASSETS_DIR" ]]; then
  echo "Assets directory not found: $ASSETS_DIR" >&2
  exit 1
fi

mapfile -t ASSETS < <(find "$ASSETS_DIR" -maxdepth 1 -type f \( -name '*.deb' -o -name '*.dmg' -o -name '*.zip' \) | sort)
if [[ ${#ASSETS[@]} -eq 0 ]]; then
  echo "No .deb/.dmg/.zip assets found in $ASSETS_DIR" >&2
  exit 1
fi

printf 'Release assets:\n'
printf '  %s\n' "${ASSETS[@]}"

if [[ -z "${GITHUB_SHA:-}" ]]; then
  echo "GITHUB_SHA is required" >&2
  exit 1
fi

if gh release view "$TAG" >/dev/null 2>&1; then
  echo "Deleting existing release $TAG to refresh assets"
  gh release delete "$TAG" --yes --cleanup-tag
fi

gh release create "$TAG" \
  --title "$TITLE" \
  --prerelease \
  --target "$GITHUB_SHA" \
  --notes-file "$NOTES_FILE"

gh release upload "$TAG" "${ASSETS[@]}" --clobber

echo "Published https://github.com/${GITHUB_REPOSITORY}/releases/tag/${TAG}"
