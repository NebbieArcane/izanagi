#!/usr/bin/env python3
"""Audit myst.* monoliths for merge artifacts that truncate server boot loading."""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path

MYST_FILES = (
    "myst.zon",
    "myst.wld",
    "myst.mob",
    "myst.obj",
    "myst.shp",
    "myst.spe",
    "myst.dam",
    "myst.act",
    "myst.pos",
    "myst.gui",
)


@dataclass
class FileReport:
    path: Path
    hash_lines: int
    unique_vnums: int
    max_vnum: int
    issues: list[str]


def scan_file(path: Path) -> FileReport:
    text = path.read_text(encoding="latin-1", errors="replace")
    lines = text.splitlines()
    vnums: list[int] = []
    issues: list[str] = []
    last_vnum: int | None = None
    total = len(lines)
    name = path.name

    for index, line in enumerate(lines, start=1):
        stripped = line.strip()

        if name == "myst.zon" and stripped == "#$" and index < total - 2:
            issues.append(
                f"line {index}: premature '#$' (server stops loading zones here)"
            )

        if name in ("myst.mob", "myst.obj") and stripped in ("%%", "%%~") and index < total - 3:
            issues.append(
                f"line {index}: premature '{stripped}' after vnum #{last_vnum} "
                "(server stops loading here)"
            )

        if name == "myst.wld" and stripped == "#0" and index < total - 3:
            issues.append(
                f"line {index}: premature '#0' after vnum #{last_vnum} "
                "(server may stop loading rooms here)"
            )

        match = re.match(r"#(\d+)", stripped)
        if match:
            vnum = int(match.group(1))
            vnums.append(vnum)
            last_vnum = vnum

    return FileReport(
        path=path,
        hash_lines=len(vnums),
        unique_vnums=len(set(vnums)),
        max_vnum=max(vnums) if vnums else 0,
        issues=issues,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("lib_dir", type=Path, help="Path to mudroot/lib")
    parser.add_argument(
        "--compare",
        type=Path,
        help="Optional second lib directory (e.g. backup) for entry counts",
    )
    args = parser.parse_args()

    lib_dir = args.lib_dir
    if not lib_dir.is_dir():
        raise SystemExit(f"not a directory: {lib_dir}")

    reports: list[FileReport] = []
    print(f"Auditing {lib_dir.resolve()}\n")

    for name in MYST_FILES:
        path = lib_dir / name
        if not path.is_file():
            continue
        report = scan_file(path)
        reports.append(report)
        dupes = report.hash_lines - report.unique_vnums
        print(f"{name}:")
        print(f"  # entries: {report.hash_lines} (unique {report.unique_vnums}, dupes {dupes})")
        print(f"  max vnum:  {report.max_vnum}")
        if report.issues:
            for issue in report.issues:
                print(f"  ISSUE: {issue}")
        else:
            print("  terminators: OK")
        print()

    if args.compare and args.compare.is_dir():
        print(f"Compare with {args.compare.resolve()}\n")
        for name in MYST_FILES:
            left = lib_dir / name
            right = args.compare / name
            if not left.is_file() or not right.is_file():
                continue
            l = scan_file(left)
            r = scan_file(right)
            delta = l.hash_lines - r.hash_lines
            sign = "+" if delta > 0 else ""
            print(
                f"{name}: {l.hash_lines} vs {r.hash_lines} ({sign}{delta}), "
                f"max vnum {l.max_vnum} vs {r.max_vnum}"
            )

    issue_count = sum(len(report.issues) for report in reports)
    return 1 if issue_count else 0


if __name__ == "__main__":
    raise SystemExit(main())
