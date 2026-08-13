#!/usr/bin/env python3
"""Build hicolor PNG icons for Linux desktop integration."""

from __future__ import annotations

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Pillow is required: pip install pillow") from exc

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "nebbie-qt" / "icons" / "nebbieedit-1024.png"
OUT_DIR = ROOT / "nebbie-qt" / "icons" / "hicolor"

SIZES = (16, 22, 24, 32, 48, 64, 128, 256, 512, 1024)


def main() -> int:
    src = SRC if len(sys.argv) < 2 else Path(sys.argv[1])
    out_dir = OUT_DIR if len(sys.argv) < 3 else Path(sys.argv[2])
    if not src.exists():
        print(f"Source icon not found: {src}", file=sys.stderr)
        return 1

    image = Image.open(src).convert("RGBA")
    for size in SIZES:
        target = out_dir / f"{size}x{size}" / "apps" / "nebbieedit.png"
        target.parent.mkdir(parents=True, exist_ok=True)
        resized = image.resize((size, size), Image.Resampling.LANCZOS)
        resized.save(target, format="PNG")
        print(f"Wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
