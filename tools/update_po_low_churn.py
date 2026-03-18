#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Apply gettext-updated PO content with low layout churn.

This script compares an original PO file against a normalized PO file
(typically produced by msgmerge/msgattrib) and patches only changed entry
blocks in the original file. Unchanged entries keep their exact on-disk
layout/comments.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import polib


POKey = tuple[str | None, str, str | None]


@dataclass
class EntryBlock:
  start: int
  end: int
  lines: list[str]


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
      description=(
          "Patch original PO with normalized gettext output while preserving "
          "layout for unchanged entries."
      )
  )
  parser.add_argument("--original", required=True, help="Original PO file path")
  parser.add_argument(
      "--normalized",
      required=True,
      help="Normalized PO file path (from msgmerge/msgattrib)",
  )
  return parser.parse_args()


def entry_key(entry: polib.POEntry) -> POKey:
  msgid_plural = entry.msgid_plural if entry.msgid_plural else None
  return (entry.msgctxt, entry.msgid, msgid_plural)


def entry_signature(entry: polib.POEntry) -> tuple:
  """Semantic signature for change detection."""
  plural_items = tuple(
      sorted((int(k), v) for k, v in entry.msgstr_plural.items())
  )
  return (
      entry.msgctxt,
      entry.msgid,
      entry.msgid_plural or None,
      entry.msgstr,
      plural_items,
      tuple(sorted(entry.flags)),
      tuple(entry.occurrences),
      entry.comment,
      entry.tcomment,
      entry.previous_msgctxt,
      entry.previous_msgid,
      entry.previous_msgid_plural,
      bool(entry.obsolete),
  )


def find_block_span(lines: list[str], linenum: int) -> tuple[int, int]:
  """
  Return [start, end) of entry block around `linenum` (1-based).

  Includes one trailing separator blank line when present.
  """
  i = max(0, linenum - 1)
  n = len(lines)
  while i > 0 and lines[i - 1].strip() != "":
    i -= 1
  start = i

  j = max(0, linenum - 1)
  while j < n and lines[j].strip() != "":
    j += 1
  end = j
  if end < n:
    end += 1

  return start, end


def build_blocks(path: Path, po: polib.POFile) -> tuple[list[str], dict[POKey, EntryBlock]]:
  with path.open("r", encoding="utf-8", newline="") as f:
    lines = f.read().splitlines(keepends=True)
  blocks: dict[POKey, EntryBlock] = {}

  for entry in po:
    if entry.msgid is None:
      continue
    key = entry_key(entry)
    start, end = find_block_span(lines, entry.linenum)
    blocks[key] = EntryBlock(start=start, end=end, lines=lines[start:end])

  return lines, blocks


def append_new_blocks(lines: list[str], block_lines: Iterable[list[str]]) -> None:
  if lines and lines[-1].strip() != "":
    lines.append("\n")
  for chunk in block_lines:
    lines.extend(chunk)
    if lines and lines[-1].strip() != "":
      lines.append("\n")


def main() -> int:
  args = parse_args()
  original_path = Path(args.original)
  normalized_path = Path(args.normalized)

  original_po = polib.pofile(str(original_path))
  normalized_po = polib.pofile(str(normalized_path))

  original_map = {entry_key(e): e for e in original_po if e.msgid is not None}
  normalized_map = {entry_key(e): e for e in normalized_po if e.msgid is not None}

  original_lines, original_blocks = build_blocks(original_path, original_po)
  _, normalized_blocks = build_blocks(normalized_path, normalized_po)

  keys_original = set(original_map)
  keys_normalized = set(normalized_map)

  keys_removed = keys_original - keys_normalized
  keys_added = keys_normalized - keys_original

  keys_changed: set[POKey] = set()
  for key in keys_original & keys_normalized:
    if entry_signature(original_map[key]) != entry_signature(normalized_map[key]):
      keys_changed.add(key)

  replacements: list[tuple[int, int, list[str]]] = []
  for key in keys_removed:
    block = original_blocks.get(key)
    if block is not None:
      replacements.append((block.start, block.end, []))

  for key in keys_changed:
    old_block = original_blocks.get(key)
    new_block = normalized_blocks.get(key)
    if old_block is not None and new_block is not None:
      replacements.append((old_block.start, old_block.end, new_block.lines))

  for start, end, new_lines in sorted(replacements, key=lambda x: x[0], reverse=True):
    original_lines[start:end] = new_lines

  if keys_added:
    added_sorted = sorted(
        keys_added, key=lambda key: normalized_map[key].linenum
    )
    append_new_blocks(
        original_lines,
        [normalized_blocks[key].lines for key in added_sorted if key in normalized_blocks],
    )

  with original_path.open("w", encoding="utf-8", newline="") as f:
    f.write("".join(original_lines))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
