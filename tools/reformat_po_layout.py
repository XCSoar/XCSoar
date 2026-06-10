#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
"""Re-apply PO semantic content while preserving an older on-disk layout."""

from __future__ import annotations

import argparse
import glob
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import polib

SCRIPT_DIR = Path(__file__).resolve().parent


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            'Patch PO files with content from one git revision using the '
            'on-disk layout from another revision (via update_po_low_churn).'
        )
    )
    parser.add_argument(
        '--layout',
        default='c335a98535',
        help='Git revision for layout baseline (default: c335a98535)',
    )
    parser.add_argument(
        '--content',
        default='HEAD',
        help='Git revision for translation content (default: HEAD)',
    )
    parser.add_argument(
        'po_files',
        nargs='*',
        help='PO files to process (default: po/*.po)',
    )
    return parser.parse_args()


def git_show(revision: str, path: str) -> bytes:
    return subprocess.check_output(['git', 'show', f'{revision}:{path}'])


def write_layout(path: str, layout_rev: str) -> None:
    data = git_show(layout_rev, path)
    Path(path).write_bytes(data)
    subprocess.run(
        [sys.executable, str(SCRIPT_DIR / 'fix_weglide_po_orphan.py'), path],
        check=True,
    )
    result = subprocess.run(['msgfmt', '--check', path], capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(
            f'{path} invalid after layout fix:\n'
            f'{result.stderr.decode()}'
        )


def reformat_file(path: str, layout_rev: str, content_rev: str) -> None:
    content_bytes = git_show(content_rev, path)
    content_po = polib.pofile(content_bytes.decode('utf-8'))

    write_layout(path, layout_rev)

    with tempfile.NamedTemporaryFile(
        mode='w', suffix='.po', delete=False, encoding='utf-8'
    ) as tmp:
        normalized_path = tmp.name
    try:
        content_po.save(normalized_path)
        subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / 'update_po_low_churn.py'),
                '--original',
                path,
                '--normalized',
                normalized_path,
            ],
            check=True,
        )
    finally:
        os.unlink(normalized_path)


def main() -> int:
    args = parse_args()
    paths = args.po_files or sorted(glob.glob('po/*.po'))
    for path in paths:
        print(f'reformat {path}')
        reformat_file(path, args.layout, args.content)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
