#!/usr/bin/env python3
"""Build hicolor PNG icons for Linux (delegates to generate-nebbie-icons.py)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "scripts" / "generate-nebbie-icons.py"


def main() -> int:
    cmd = [sys.executable, str(GENERATOR), "nebbieedit"]
    return subprocess.call(cmd)


if __name__ == "__main__":
    raise SystemExit(main())
