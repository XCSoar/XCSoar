#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
"""Fix orphaned WeGlide msgstr blocks left by a bad msgmerge."""

from __future__ import annotations

import re
import sys
from pathlib import Path

# msgstr without msgid between Nearest Airspace and Select WeGlide Type.
ORPHAN_RE = re.compile(
    r'(#: Data/Input/default\.xci:1390\n'
    r'msgid "Nearest Airspace"\n'
    r'(?:msgstr "[^"]*"\n|msgstr ""\n(?:"[^"]*"\n)+)'
    r')\nmsgstr "([^"]*)"\n\n'
    r'(#: src/Dialogs/Plane/WeGlideTypePicker\.cpp:99)',
    re.MULTILINE,
)

INSERT = (
    '\n#: src/Dialogs/Plane/WeGlideTypePicker.cpp:78\n'
    '#: src/Dialogs/Plane/WeGlideTypePicker.cpp:116\n'
    'msgid "Could not load WeGlide aircraft list."\n'
    'msgstr "{}"\n\n'
)


def fix_content(text: str) -> tuple[str, bool]:
    if not ORPHAN_RE.search(text):
        return text, False
    return ORPHAN_RE.sub(lambda m: m.group(1) + INSERT.format(m.group(2)) + m.group(3),
                         text), True


def fix_file(path: Path) -> bool:
    text = path.read_text(encoding='utf-8')
    fixed, changed = fix_content(text)
    if changed:
        path.write_text(fixed, encoding='utf-8', newline='')
    return changed


def main() -> int:
    changed = 0
    for path in map(Path, sys.argv[1:]):
        if fix_file(path):
            changed += 1
            print(f'fixed {path}')
    print(f'fixed {changed} file(s)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
