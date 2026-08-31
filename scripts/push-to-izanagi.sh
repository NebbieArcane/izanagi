#!/usr/bin/env bash
# Push the full repository to NebbieArcane/izanagi.
# Auth order: IZANAGI_DEPLOY_KEY secret, then host SSH agent, then HTTPS.
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

if [[ -n "${IZANAGI_DEPLOY_KEY:-}" ]]; then
  # shellcheck source=/dev/null
  source "$(dirname "$0")/setup-izanagi-deploy-key.sh"
fi

git_ssh() {
  GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null \
    GIT_SSH_COMMAND='ssh -o StrictHostKeyChecking=accept-new' \
    git "$@"
}

ssh_ready() {
  [[ -n "${SSH_AUTH_SOCK:-}" && -S "$SSH_AUTH_SOCK" ]] && ssh-add -l >/dev/null 2>&1
}

github_ssh_ok() {
  local output
  output="$(ssh -o BatchMode=yes -o StrictHostKeyChecking=accept-new \
    -T git@github.com 2>&1 || true)"
  grep -qi 'successfully authenticated' <<<"$output"
}

push_all() {
  local target="$1"
  local mode="${2:-https}"
  echo "Pushing branches to $target ..."
  if [[ "$mode" == ssh ]]; then
    git_ssh push "$target" --all
    if git tag -l | grep -q .; then
      echo "Pushing tags ..."
      git_ssh push "$target" --tags
    fi
  else
    git push "$target" --all
    if git tag -l | grep -q .; then
      echo "Pushing tags ..."
      git push "$target" --tags
    fi
  fi
}

if [[ "$USE_SSH" == "1" || ( "$USE_SSH" == "auto" && "$(ssh_ready && echo yes || echo no)" == "yes" ) ]]; then
  echo "Using SSH agent at ${SSH_AUTH_SOCK:-<unset>}"
  ssh-add -l || true
  if github_ssh_ok; then
    push_all "$REPO_SSH" ssh
    echo "OK: https://github.com/NebbieArcane/izanagi"
    exit 0
  fi
  if [[ -n "${IZANAGI_DEPLOY_KEY:-}" ]]; then
    echo "Deploy key presente ma GitHub non autentica." >&2
    exit 1
  fi
  echo "La chiave nell'agente SSH non è la tua: è la chiave interna di Cursor," >&2
  echo "già registrata su GitHub e non autorizzata su NebbieArcane/izanagi." >&2
  echo "Usa una deploy key dedicata (vedi README / messaggio agent)." >&2
  ssh-add -L >&2 || true
  exit 1
fi

if ! git remote get-url "$REMOTE_NAME" >/dev/null 2>&1; then
  git remote add "$REMOTE_NAME" "$REPO_HTTPS"
else
  git remote set-url "$REMOTE_NAME" "$REPO_HTTPS"
fi
push_all "$REMOTE_NAME" https
echo "OK: https://github.com/NebbieArcane/izanagi"
