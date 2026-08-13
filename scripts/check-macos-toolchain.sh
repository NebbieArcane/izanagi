#!/usr/bin/env bash
# Verify macOS can compile standard C++ headers (catches missing/broken Xcode CLT).
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
    exit 0
fi

fail() {
    cat >&2 <<'EOF'

macOS toolchain check FAILED.

The error 'cstdio file not found' (or similar) means the C++ compiler cannot
see Apple system headers. This is not a Nebbie Editor bug.

If xcode-select -p shows CommandLineTools but clang++ still fails, the CLT
install is incomplete — reinstall from scratch (see step 2).

Fix (try in order):

  1. Diagnose (paste output if you need help):
       xcode-select -p
       which clang++
       clang++ --version
       ls /Library/Developer/CommandLineTools/SDKs/
       ls /Library/Developer/CommandLineTools/usr/include/c++/v1/cstdio 2>&1

  2. Reinstall Command Line Tools completely:
       sudo rm -rf /Library/Developer/CommandLineTools
       xcode-select --install
     Wait for the GUI installer to finish, open a NEW terminal, then re-run step 1.

  3. Or install full Xcode from the App Store, then:
       sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
       sudo xcodebuild -runFirstLaunch

  4. Point xcode-select (only if step 2/3 already installed tools):
       sudo xcode-select -s /Library/Developer/CommandLineTools
     or:
       sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

  Note: 'sudo xcodebuild -license accept' works only with full Xcode.app,
  not with Command Line Tools alone — skip it if you only use CLT.

  5. Clean and rebuild Nebbie Editor:
       rm -rf build
       ./scripts/build.sh

Verify manually:
       clang++ -std=c++17 -x c++ -c - -o /dev/null <<< '#include <cstdio>'
       echo $?    # must print 0

EOF
    exit 1
}

if ! command -v clang++ >/dev/null 2>&1; then
    echo "ERROR: clang++ not found." >&2
    fail
fi

dev_path="$(xcode-select -p 2>/dev/null || true)"
if [[ -z "${dev_path}" || ! -d "${dev_path}" ]]; then
    echo "ERROR: xcode-select has no valid developer directory." >&2
    fail
fi

if ! clang++ -std=c++17 -x c++ -c - -o /dev/null 2>/dev/null <<< '#include <cstdio>'; then
    echo "ERROR: clang++ cannot compile <cstdio> (developer dir: ${dev_path})" >&2
    fail
fi

echo "macOS toolchain OK (${dev_path})"
