#!/usr/bin/env bash
# =============================================================================
# Eleutheria — sync dal VPS + build su macOS
#
# Uso tipico (dal Mac):
#   ~/src/new-izanagi/scripts/mac-sync-build.sh
#   ~/src/new-izanagi/scripts/mac-sync-build.sh --open
#
# Prima volta (se non hai ancora il repo in locale), dal Mac:
#   curl -fsSL non serve: copia UNA volta con:
#     mkdir -p ~/src && rsync -avz vps-tailscale:/home/nebbie/new-izanagi/scripts/mac-sync-build.sh /tmp/
#     bash /tmp/mac-sync-build.sh --bootstrap
#   oppure esegui questo script se e' gia' sul Mac.
#
# Config (override con env o file ~/.config/eleutheria/sync.env):
#   ELEUTHERIA_VPS=vps-tailscale
#   ELEUTHERIA_REMOTE=/home/nebbie/new-izanagi
#   ELEUTHERIA_LOCAL=$HOME/src/new-izanagi
# =============================================================================
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
DO_BOOTSTRAP=0
DO_SYNC=1
DO_BUILD=1
DO_OPEN=0
DO_ONLY_OPEN=0
DO_TEST=1
DRY_RUN=0

usage() {
  cat <<EOF
${SCRIPT_NAME} — sincronizza Eleutheria dal VPS e compila sul Mac

Uso:
  ${SCRIPT_NAME} [opzioni]

Opzioni:
  --bootstrap   Prima installazione: crea cartella locale, sync, deps, build
  --open        Dopo la build, avvia eleutheria.app
  --only-open   Solo avvia l'app (niente sync, niente rebuild)
  --no-build    Solo sync, non compilare
  --no-test     Build senza ctest
  --dry-run     Mostra i comandi senza eseguirli
  -h, --help    Questa guida

Default (modificabili con env o ~/.config/eleutheria/sync.env):
  ELEUTHERIA_VPS=${ELEUTHERIA_VPS:-vps-tailscale}
  ELEUTHERIA_REMOTE=${ELEUTHERIA_REMOTE:-/home/nebbie/new-izanagi}
  ELEUTHERIA_LOCAL=${ELEUTHERIA_LOCAL:-\$HOME/src/new-izanagi}

Esempi:
  ${SCRIPT_NAME}              # sync + build
  ${SCRIPT_NAME} --open       # sync + build + avvia
  ${SCRIPT_NAME} --only-open  # solo avvia (rapido)
  ${SCRIPT_NAME} --no-build   # solo sync
EOF
}

# Carica config utente se presente
CONFIG_FILE="${ELEUTHERIA_SYNC_ENV:-$HOME/.config/eleutheria/sync.env}"
if [[ -f "${CONFIG_FILE}" ]]; then
  # shellcheck disable=SC1090
  source "${CONFIG_FILE}"
fi

ELEUTHERIA_VPS="${ELEUTHERIA_VPS:-vps-tailscale}"
ELEUTHERIA_REMOTE="${ELEUTHERIA_REMOTE:-/home/nebbie/new-izanagi}"
ELEUTHERIA_LOCAL="${ELEUTHERIA_LOCAL:-$HOME/src/new-izanagi}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --bootstrap) DO_BOOTSTRAP=1 ;;
    --open) DO_OPEN=1 ;;
    --only-open) DO_ONLY_OPEN=1; DO_SYNC=0; DO_BUILD=0; DO_OPEN=1 ;;
    --no-build) DO_BUILD=0 ;;
    --no-test) DO_TEST=0 ;;
    --dry-run) DRY_RUN=1 ;;
    -h|--help) usage; exit 0 ;;
    *)
      echo "Opzione sconosciuta: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

run() {
  if [[ "${DRY_RUN}" -eq 1 ]]; then
    printf '[dry-run]'
    printf ' %q' "$@"
    printf '\n'
    return 0
  fi
  "$@"
}

log() { printf '==> %s\n' "$*"; }
die() { printf 'ERRORE: %s\n' "$*" >&2; exit 1; }

if [[ "$(uname -s)" != "Darwin" ]]; then
  die "Questo script e' pensato per macOS (sei su $(uname -s))."
fi

# Diagnostica: utile se eleutheria-sync e lo script "diretto" sembrano diversi
SCRIPT_REAL="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
log "Script: ${SCRIPT_REAL}"
log "Local:  ${ELEUTHERIA_LOCAL}"
log "Remote: ${ELEUTHERIA_VPS}:${ELEUTHERIA_REMOTE}"
log "Config: ${CONFIG_FILE}"

APP="${ELEUTHERIA_LOCAL}/build/app/eleutheria.app"

# Percorso rapido: solo aprire l'app gia' compilata
if [[ "${DO_ONLY_OPEN}" -eq 1 ]]; then
  [[ -d "${APP}" ]] || die "App non trovata: ${APP}
Compila almeno una volta con: ${SCRIPT_NAME}"
  log "Avvio Eleutheria (senza sync/build)"
  run open "${APP}"
  exit 0
fi

command -v rsync >/dev/null || die "rsync non trovato"
command -v ssh >/dev/null || die "ssh non trovato"

if [[ "${DO_SYNC}" -eq 1 ]] || [[ "${DO_BOOTSTRAP}" -eq 1 ]]; then
  log "Verifica SSH verso ${ELEUTHERIA_VPS} ..."
  if [[ "${DRY_RUN}" -eq 0 ]]; then
    if ! ssh -o BatchMode=yes -o ConnectTimeout=8 "${ELEUTHERIA_VPS}" "test -d '${ELEUTHERIA_REMOTE}'"; then
      die "SSH fallito verso '${ELEUTHERIA_VPS}' oppure manca ${ELEUTHERIA_REMOTE}.
Controlla ~/.ssh/config (Host vps-tailscale + IdentityFile) e che il VPS sia raggiungibile."
    fi
  fi
fi

if [[ "${DO_BOOTSTRAP}" -eq 1 ]] || [[ ! -d "${ELEUTHERIA_LOCAL}" ]]; then
  log "Creo ${ELEUTHERIA_LOCAL}"
  run mkdir -p "${ELEUTHERIA_LOCAL}"
fi

if [[ "${DO_BOOTSTRAP}" -eq 1 ]]; then
  if ! command -v brew >/dev/null 2>&1; then
    die "Homebrew non trovato. Installa da https://brew.sh poi rilancia."
  fi
  log "Dipendenze Homebrew (qt, cmake)"
  if [[ "${DRY_RUN}" -eq 0 ]]; then
    brew list qt >/dev/null 2>&1 || brew list qt@6 >/dev/null 2>&1 || brew install qt
    brew list cmake >/dev/null 2>&1 || brew install cmake
  else
    run brew install qt cmake
  fi
fi

if [[ "${DO_SYNC}" -eq 1 ]]; then
  log "Sync ${ELEUTHERIA_VPS}:${ELEUTHERIA_REMOTE}/ -> ${ELEUTHERIA_LOCAL}/"
  run mkdir -p "${ELEUTHERIA_LOCAL}"
  run rsync -avz --delete \
    --exclude build \
    --exclude eleutheria-src \
    --exclude '.git' \
    --exclude '*.user' \
    --exclude '.DS_Store' \
    --exclude '**/.*DS_Store' \
    "${ELEUTHERIA_VPS}:${ELEUTHERIA_REMOTE}/" "${ELEUTHERIA_LOCAL}/"

  run chmod +x "${ELEUTHERIA_LOCAL}/scripts/"*.sh 2>/dev/null || true

  if [[ "${DRY_RUN}" -eq 0 ]]; then
    mkdir -p "$(dirname "${CONFIG_FILE}")"
    cat > "${CONFIG_FILE}" <<EOF
# Generato da ${SCRIPT_NAME} — modifica pure
ELEUTHERIA_VPS="${ELEUTHERIA_VPS}"
ELEUTHERIA_REMOTE="${ELEUTHERIA_REMOTE}"
ELEUTHERIA_LOCAL="${ELEUTHERIA_LOCAL}"
EOF
    log "Config salvata in ${CONFIG_FILE}"
  fi
fi

if [[ "${DO_BUILD}" -eq 1 ]]; then
  log "Build in ${ELEUTHERIA_LOCAL}"
  if [[ "${DO_TEST}" -eq 1 ]]; then
    run bash "${ELEUTHERIA_LOCAL}/scripts/build.sh"
  else
    (
      cd "${ELEUTHERIA_LOCAL}"
      if [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
        for formula in qt qt@6 qt6; do
          if brew --prefix "${formula}" >/dev/null 2>&1; then
            candidate="$(brew --prefix "${formula}")"
            if [[ -d "${candidate}/lib/cmake/Qt6" ]]; then
              export CMAKE_PREFIX_PATH="${candidate}"
              break
            fi
          fi
        done
      fi
      run cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ${CMAKE_PREFIX_PATH:+-DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}"}
      run cmake --build build -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    )
  fi
  if [[ -d "${APP}" ]] && [[ "${DRY_RUN}" -eq 0 ]]; then
    xattr -dr com.apple.quarantine "${APP}" 2>/dev/null || true
  fi
  log "App: ${APP}"
elif [[ "${DO_SYNC}" -eq 1 ]]; then
  log "Sync completato (--no-build). Codice in ${ELEUTHERIA_LOCAL}"
fi

if [[ "${DO_OPEN}" -eq 1 ]]; then
  [[ -d "${APP}" ]] || die "App non trovata: ${APP} (serve una build)"
  log "Avvio Eleutheria"
  run open "${APP}"
fi

log "Fatto."
printf '\nComandi utili:\n  %s              # sync + build\n  %s --open       # sync + build + avvia\n  %s --only-open  # solo avvia\n' \
  "${ELEUTHERIA_LOCAL}/scripts/mac-sync-build.sh" \
  "${ELEUTHERIA_LOCAL}/scripts/mac-sync-build.sh" \
  "${ELEUTHERIA_LOCAL}/scripts/mac-sync-build.sh"
