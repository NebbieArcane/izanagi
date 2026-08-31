#!/usr/bin/env bash
# Push the full repository to NebbieArcane/izanagi.
# Prefers SSH via the host SSH agent when available (Cloud Agent: SSH_AUTH_SOCK).
set -euo pipefail

REPO_SSH="${IZANAGI_REPO_SSH:-git@github.com:NebbieArcane/izanagi.git}"
REPO_HTTPS="${IZANAGI_REPO_HTTPS:-https://github.com/NebbieArcane/izanagi.git}"
REMOTE_NAME="${IZANAGI_REMOTE_NAME:-izanagi}"
USE_SSH="${IZANAGI_USE_SSH:-auto}"

cd "$(git rev-parse --show-toplevel)"

if [[ -z "${SSH_AUTH_SOCK:-}" && -S /run/host-services/ssh-auth.sock ]]; then
  export SSH_AUTH_SOCK=/run/host-services/ssh-auth.sock
elif [[ -n "${SSH_AUTH_SOCK:-}" && ! -S "$SSH_AUTH_SOCK" && -S /run/host-services/ssh-auth.sock ]]; then
  export SSH_AUTH_SOCK=/run/host-services/ssh-auth.sock
fi

git_ssh() {
  GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null \
    GIT_SSH_COMMAND='ssh -o StrictHostKeyChecking=accept-new' \
    git "$@"
}

ssh_ready() {
  [[ -n "${SSH_AUTH_SOCK:-}" && -S "$SSH_AUTH_SOCK" ]] && ssh-add -l >/dev/null 2>&1
}

push_all() {
  local target="$1"
  shift
  echo "Pushing branches to $target ..."
  git "$@" push "$target" --all
  if git tag -l | grep -q .; then
    echo "Pushing tags ..."
    git "$@" push "$target" --tags
  fi
}

if [[ "$USE_SSH" == "1" || ( "$USE_SSH" == "auto" && "$(ssh_ready && echo yes || echo no)" == "yes" ) ]]; then
  echo "Using SSH agent at ${SSH_AUTH_SOCK:-<unset>}"
  ssh-add -l || true
  if ! ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new -T git@github.com 2>&1 | grep -qi 'successfully authenticated'; then
    echo "GitHub non accetta la chiave SSH caricata." >&2
    echo "Aggiungi questa chiave pubblica su https://github.com/settings/keys :" >&2
    ssh-add -L >&2 || true
    exit 1
  fi
  push_all "$REPO_SSH" git_ssh
else
  if ! git remote get-url "$REMOTE_NAME" >/dev/null 2>&1; then
    git remote add "$REMOTE_NAME" "$REPO_HTTPS"
  else
    git remote set-url "$REMOTE_NAME" "$REPO_HTTPS"
  fi
  push_all "$REMOTE_NAME" git
fi

echo "OK: https://github.com/NebbieArcane/izanagi"
