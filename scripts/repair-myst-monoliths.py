#!/usr/bin/env python3
"""Remove premature myst monolith terminators introduced by bad Aree merges.

Prefer `nebbiedit repair-lib <lib-directory>` which also rewrites myst.wld for
server-compatible TUNNEL room export.
"""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

TERMINATORS = {
    "myst.zon": ("#$",),
    "myst.mob": ("%%", "%%~"),
    "myst.obj": ("%%", "%%~"),
}


def repair_file(path: Path, markers: tuple[str, ...], dry_run: bool) -> int:
    lines = path.read_text(encoding="latin-1", errors="replace").splitlines(keepends=True)
    terminator_indexes: list[int] = []
    for index, line in enumerate(lines):
        if line.rstrip("\r\n") in markers:
            terminator_indexes.append(index)

    if len(terminator_indexes) <= 1:
        return 0

    # Keep only the final terminator; earlier ones truncate server loading.
    remove_indexes = set(terminator_indexes[:-1])
    if dry_run:
        print(f"{path}: would remove {len(remove_indexes)} premature terminator line(s)")
        for index in terminator_indexes[:-1]:
            print(f"  line {index + 1}: {lines[index].rstrip()!r}")
        return len(remove_indexes)

    backup = path.with_suffix(path.suffix + ".bak")
    if not backup.exists():
        shutil.copy2(path, backup)

    kept = [line for index, line in enumerate(lines) if index not in remove_indexes]
    path.write_text("".join(kept), encoding="latin-1", errors="replace", newline="")
    print(f"{path}: removed {len(remove_indexes)} premature terminator line(s)")
    print(f"Backup: {backup}")
    return len(remove_indexes)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("lib_dir", type=Path, help="Path to mudroot/lib")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    lib_dir = args.lib_dir
    if not lib_dir.is_dir():
        raise SystemExit(f"not a directory: {lib_dir}")

    removed_total = 0
    for name, markers in TERMINATORS.items():
        path = lib_dir / name
        if not path.is_file():
            continue
        removed_total += repair_file(path, markers, args.dry_run)

    return 0 if removed_total == 0 else 0


if __name__ == "__main__":
    raise SystemExit(main())
