#!/usr/bin/env python3
"""Remove standalone TUNNEL moblim lines from myst.wld for NebbieArcane server boot."""

from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path

ZONE_LINE = re.compile(r"^-?\d+(\s+-?\d+(\|-?\d+)*)+\s+-?\d+")


def is_zone_line(line: str) -> bool:
    return bool(ZONE_LINE.match(line.strip()))


def is_moblim_line(line: str) -> bool:
    stripped = line.strip()
    return stripped.isdigit() and len(stripped) <= 4


def is_aux_line(line: str) -> bool:
    stripped = line.strip()
    return bool(stripped) and stripped[0] in "DELCS"


def repair_text(text: str) -> tuple[str, int]:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    removed = 0
    index = 0

    while index < len(lines):
        current = lines[index]
        stripped = current.rstrip("\r\n")

        if (
            is_zone_line(stripped)
            and index + 2 < len(lines)
            and is_moblim_line(lines[index + 1].rstrip("\r\n"))
            and is_aux_line(lines[index + 2].rstrip("\r\n"))
        ):
            out.append(current)
            index += 2
            removed += 1
            continue

        out.append(current)
        index += 1

    return "".join(out), removed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wld_path", type=Path, help="Path to myst.wld")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Report how many moblim lines would be removed without writing",
    )
    args = parser.parse_args()

    path = args.wld_path
    if not path.is_file():
        raise SystemExit(f"file not found: {path}")

    original = path.read_text(encoding="latin-1", errors="replace")
    repaired, removed = repair_text(original)

    if args.dry_run:
        print(f"Would remove {removed} standalone moblim line(s) from {path}")
        return 0

    if removed == 0:
        print(f"No standalone moblim lines found in {path}")
        return 0

    backup = path.with_suffix(path.suffix + ".bak")
    shutil.copy2(path, backup)
    path.write_text(repaired, encoding="latin-1", errors="replace", newline="")
    print(f"Removed {removed} standalone moblim line(s) from {path}")
    print(f"Backup: {backup}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
