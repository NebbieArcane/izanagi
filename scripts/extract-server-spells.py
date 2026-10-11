#!/usr/bin/env python3
"""Extract spells[] from NebbieArcane/Server spell_parser.cpp into izanagi .inc format."""
import re
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} spell_parser.cpp", file=sys.stderr)
        return 1
    text = Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace")
    start = text.find("const char* spells[]")
    if start < 0:
        print("spells[] not found", file=sys.stderr)
        return 1
    end = text.find("\n};", start)
    body = text[start:end]
    spells = []
    for line in body.splitlines():
        m = re.search(r'"([^"]*)"', line)
        if m:
            spells.append(m.group(1))
    print("// Generated from NebbieArcane/Server spell_parser.cpp (read-only reference)")
    for i, name in enumerate(spells):
        esc = name.replace("\\", "\\\\").replace('"', '\\"')
        print(f'    {{{i + 1}, "{esc}"}},')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
