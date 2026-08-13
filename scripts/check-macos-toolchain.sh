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

Fix (try in order):

  1. Install or reinstall Xcode Command Line Tools:
       xcode-select --install

  2. Point xcode-select at the CLT or full Xcode:
       sudo xcode-select -s /Library/Developer/CommandLineTools
     or, if you use Xcode.app:
       sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

  3. Accept the Xcode license (if prompted):
       sudo xcodebuild -license accept

  4. After a macOS upgrade, reset and reinstall:
       sudo xcode-select --reset
       xcode-select --install

  5. Clean and rebuild Nebbie Editor:
       rm -rf build
       ./scripts/build.sh

Verify manually:
       xcode-select -p
       clang++ -std=c++17 -x c++ -c - -o /dev/null <<< '#include <cstdio>'

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
