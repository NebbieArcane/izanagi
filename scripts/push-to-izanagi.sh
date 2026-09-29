#!/usr/bin/env bash
# Push main (and tags) to origin (NebbieArcane/izanagi).
# Cloud Agents: set IZANAGI_DEPLOY_KEY for SSH deploy-key auth when HTTPS is unavailable.
set -euo pipefail

REPO_SSH="${IZANAGI_REPO_SSH:-git@github.com:NebbieArcane/izanagi.git}"
REMOTE_NAME="${IZANAGI_REMOTE_NAME:-origin}"
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

push_main() {
  local remote="$1"
  local mode="${2:-https}"
  echo "Pushing main to $remote ..."
  if [[ "$mode" == ssh ]]; then
    git_ssh push "$remote" main:main
    if git tag -l | grep -q .; then
      echo "Pushing tags ..."
      git_ssh push "$remote" --tags
    fi
  else
    git push "$remote" main:main
    if git tag -l | grep -q .; then
      echo "Pushing tags ..."
      git push "$remote" --tags
    fi
  fi
}

if [[ "$USE_SSH" == "1" || ( "$USE_SSH" == "auto" && "$(ssh_ready && echo yes || echo no)" == "yes" ) ]]; then
  echo "Using SSH agent at ${SSH_AUTH_SOCK:-<unset>}"
  ssh-add -l || true
  if github_ssh_ok; then
    if [[ "$REMOTE_NAME" == origin ]]; then
      git remote set-url origin "$REPO_SSH"
    fi
    push_main "$REMOTE_NAME" ssh
    echo "OK: https://github.com/NebbieArcane/izanagi"
    exit 0
  fi
  if [[ -n "${IZANAGI_DEPLOY_KEY:-}" ]]; then
    echo "Deploy key presente ma GitHub non autentica." >&2
    exit 1
  fi
  echo "SSH non disponibile per NebbieArcane/izanagi; usa HTTPS su origin o IZANAGI_DEPLOY_KEY." >&2
  exit 1
fi

push_main "$REMOTE_NAME" https
echo "OK: https://github.com/NebbieArcane/izanagi"
