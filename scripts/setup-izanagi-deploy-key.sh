#!/usr/bin/env bash
# Load IZANAGI_DEPLOY_KEY (private key) into a dedicated ssh-agent for git push.
# Secret: Cursor dashboard → Cloud Agents → Secrets → IZANAGI_DEPLOY_KEY
set -euo pipefail

if [[ -z "${IZANAGI_DEPLOY_KEY:-}" ]]; then
  exit 1
fi

keyfile="$(mktemp)"
cleanup() { rm -f "$keyfile"; }
trap cleanup EXIT

umask 077
printf '%s\n' "$IZANAGI_DEPLOY_KEY" > "$keyfile"

if [[ -z "${SSH_AUTH_SOCK:-}" || ! -S "${SSH_AUTH_SOCK}" ]]; then
  eval "$(ssh-agent -s)" >/dev/null
fi

ssh-add "$keyfile" >/dev/null
echo "Deploy key izanagi caricata in ssh-agent."
