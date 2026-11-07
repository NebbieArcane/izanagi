#!/usr/bin/env bash
# Da incollare/eseguire UNA volta sul Mac (bootstrap dello script di sync).
# Non richiede di avere gia' il repo in locale.
set -euo pipefail

VPS="${IZANAGI_VPS:-vps-tailscale}"
REMOTE="${IZANAGI_REMOTE:-/home/nebbie/new-izanagi}"
LOCAL="${IZANAGI_LOCAL:-$HOME/src/new-izanagi}"
BIN_LINK="${HOME}/bin/izanagi-sync"

echo "==> Scarico lo script di sync da ${VPS}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

rsync -avz "${VPS}:${REMOTE}/scripts/mac-sync-build.sh" "${tmpdir}/mac-sync-build.sh"
chmod +x "${tmpdir}/mac-sync-build.sh"

echo "==> Bootstrap completo (sync + deps + build)"
IZANAGI_VPS="${VPS}" \
IZANAGI_REMOTE="${REMOTE}" \
IZANAGI_LOCAL="${LOCAL}" \
  bash "${tmpdir}/mac-sync-build.sh" --bootstrap

# Wrapper (non symlink): cosi' izanagi-sync usa SEMPRE lo script
# aggiornato in IZANAGI_LOCAL dopo ogni sync, non una copia vecchia.
mkdir -p "${HOME}/bin"
cat > "${BIN_LINK}" <<EOF
#!/usr/bin/env bash
set -euo pipefail
CONFIG="\${IZANAGI_SYNC_ENV:-\$HOME/.config/izanagi/sync.env}"
if [[ -f "\${CONFIG}" ]]; then
  # shellcheck disable=SC1090
  source "\${CONFIG}"
fi
LOCAL="\${IZANAGI_LOCAL:-${LOCAL}}"
SCRIPT="\${LOCAL}/scripts/mac-sync-build.sh"
if [[ ! -x "\${SCRIPT}" ]]; then
  echo "ERRORE: script non trovato: \${SCRIPT}" >&2
  echo "Rilancia install-mac-sync.sh oppure: bash \${LOCAL}/scripts/mac-sync-build.sh --bootstrap" >&2
  exit 1
fi
exec bash "\${SCRIPT}" "\$@"
EOF
chmod +x "${BIN_LINK}"

echo
echo "OK. Da ora, in qualsiasi terminale sul Mac:"
echo "  izanagi-sync           # se ~/bin e' nel PATH"
echo "  izanagi-sync --open    # sync + rebuild + avvia (stesso di scripts/mac-sync-build.sh --open)"
echo "  izanagi-sync --only-open  # SOLO avvia, senza sync/rebuild"
echo
if ! echo ":$PATH:" | grep -q ":$HOME/bin:"; then
  echo "Aggiungi ~/bin al PATH (zsh):"
  echo "  echo 'export PATH=\"\$HOME/bin:\$PATH\"' >> ~/.zshrc && source ~/.zshrc"
fi
