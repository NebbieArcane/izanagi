#!/usr/bin/env bash
# Configura git per usare il PAT personale quando l'app Cursor non ha push sul repo.
# Secret: https://cursor.com/dashboard → Cloud Agents → Secrets
# Nome: WIZARDMORGAN_GITHUB_PAT  (NON usare GH_TOKEN)

set -euo pipefail

PAT="${WIZARDMORGAN_GITHUB_PAT:-}"
CANONICAL_ORIGIN="https://github.com/NebbieArcane/izanagi.git"

if [[ -z "$PAT" ]]; then
  echo "WIZARDMORGAN_GITHUB_PAT non impostato: git userà cursor[bot] (può fallire sul push)."
  exit 0
fi

git config --global credential.helper store
printf 'https://wizardmorgan:%s@github.com\n' "$PAT" > "${HOME}/.git-credentials"
chmod 600 "${HOME}/.git-credentials"

if git rev-parse --is-inside-work-tree &>/dev/null; then
  origin_url="$(git remote get-url origin 2>/dev/null || true)"
  if [[ "$origin_url" == *"x-access-token"* || "$origin_url" == *"cursor"* || "$origin_url" == *"nebbie-arcane-editing-tools"* || "$origin_url" == *"nebbie-editor"* ]]; then
    git remote set-url origin "$CANONICAL_ORIGIN"
    echo "Remote origin reimpostato su NebbieArcane/izanagi."
  fi
fi

echo "Git configurato con WIZARDMORGAN_GITHUB_PAT."
