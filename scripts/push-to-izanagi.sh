#!/usr/bin/env bash
# Push the full repository to NebbieArcane/izanagi.
# Requires write access to https://github.com/NebbieArcane/izanagi
set -euo pipefail

REPO_URL="${IZANAGI_REPO_URL:-https://github.com/NebbieArcane/izanagi.git}"
REMOTE_NAME="${IZANAGI_REMOTE_NAME:-izanagi}"

cd "$(git rev-parse --show-toplevel)"

if ! git remote get-url "$REMOTE_NAME" >/dev/null 2>&1; then
  git remote add "$REMOTE_NAME" "$REPO_URL"
else
  git remote set-url "$REMOTE_NAME" "$REPO_URL"
fi

echo "Pushing branches to $REPO_URL ..."
git push -u "$REMOTE_NAME" --all

if git tag -l | grep -q .; then
  echo "Pushing tags ..."
  git push "$REMOTE_NAME" --tags
fi

echo "OK: https://github.com/NebbieArcane/izanagi"
