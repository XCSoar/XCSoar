#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Apply gettext-updated PO content with low layout churn.

Compares an original PO file against normalized gettext output and patches
only entries whose translations (or keys) changed. Preserves original
msgid/msgstr wrapping where possible, strips `#:` locations, and upgrades
1:1 msgctxt additions in place.
"""

from __future__ import annotations

import argparse
import shutil
from collections import defaultdict
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
  """Translation signature — layout/metadata-only diffs must not rewrite."""
  plural_items = tuple(
      sorted((int(k), v) for k, v in entry.msgstr_plural.items())
  )
  return (
      entry.msgstr,
      plural_items,
      bool(entry.obsolete),
  )


def strip_location_lines(lines: list[str]) -> list[str]:
  """Remove gettext `#:` occurrence lines."""
  return [line for line in lines if not line.startswith("#:")]


def po_quote(value: str) -> str:
  escaped = (
      value.replace("\\", "\\\\")
      .replace('"', '\\"')
      .replace("\n", "\\n")
  )
  return f'"{escaped}"'


def inject_msgctxt(lines: list[str], msgctxt: str) -> list[str]:
  """Insert or replace a msgctxt line before msgid, keeping body layout."""
  msgctxt_line = f"msgctxt {po_quote(msgctxt)}\n"
  out: list[str] = []
  inserted = False
  for line in lines:
    if line.startswith("msgctxt "):
      continue
    if not inserted and line.startswith("msgid"):
      out.append(msgctxt_line)
      inserted = True
    out.append(line)
  if not inserted:
    out.insert(0, msgctxt_line)
  return out


def split_entry_sections(
    lines: list[str],
) -> tuple[list[str], list[str], list[str]]:
  """Split an entry into prefix, msgid section, msgstr section."""
  prefix: list[str] = []
  i = 0
  while i < len(lines):
    line = lines[i]
    if line.startswith("msgid"):
      break
    prefix.append(line)
    i += 1

  msgid_section: list[str] = []
  while i < len(lines):
    line = lines[i]
    if line.startswith("msgstr"):
      break
    msgid_section.append(line)
    i += 1

  return prefix, msgid_section, lines[i:]


def merge_keeping_msgid(
    old_lines: list[str], new_lines: list[str]
) -> list[str]:
  """
  Build a replacement block: normalized prefix/msgstr, original msgid layout.

  Avoids rewrapping msgid when gettext/polib only changed quote line breaks.
  """
  old_lines = strip_location_lines(old_lines)
  new_lines = strip_location_lines(new_lines)
  _, old_msgid, _ = split_entry_sections(old_lines)
  new_prefix, new_msgid, new_msgstr = split_entry_sections(new_lines)
  if not old_msgid:
    return new_lines
  return new_prefix + old_msgid + new_msgstr


def entry_block_start(lines: list[str], linenum: int) -> int:
  """
  Index of the first line of the entry that contains `linenum` (1-based).

  Walk upward only through leading metadata (comments / msgctxt).
  """
  i = max(0, linenum - 1)
  while i > 0:
    prev = lines[i - 1]
    if prev.strip() == "":
      break
    if prev.startswith("#") or prev.startswith("msgctxt "):
      i -= 1
      continue
    break
  return i


def build_blocks(path: Path, po: polib.POFile) -> tuple[list[str], dict[POKey, EntryBlock]]:
  with path.open("r", encoding="utf-8", newline="") as f:
    lines = f.read().splitlines(keepends=True)
  blocks: dict[POKey, EntryBlock] = {}

  entries = sorted(
      (e for e in po if e.msgid is not None),
      key=lambda e: e.linenum,
  )
  starts = [entry_block_start(lines, e.linenum) for e in entries]

  for i, entry in enumerate(entries):
    start = starts[i]
    end = starts[i + 1] if i + 1 < len(entries) else len(lines)
    key = entry_key(entry)
    blocks[key] = EntryBlock(start=start, end=end, lines=lines[start:end])

  return lines, blocks


def append_new_blocks(lines: list[str], block_lines: Iterable[list[str]]) -> None:
  if lines and lines[-1].strip() != "":
    lines.append("\n")
  for chunk in block_lines:
    lines.extend(chunk)
    if lines and lines[-1].strip() != "":
      lines.append("\n")


def blocks_overlap(blocks: dict[POKey, EntryBlock]) -> bool:
  """True if any two entry blocks share or cross the same line range."""
  spans = sorted((b.start, b.end) for b in blocks.values())
  for (start_a, end_a), (start_b, end_b) in zip(spans, spans[1:]):
    if end_a > start_b:
      return True
  return False


def find_msgctxt_upgrades(
    keys_removed: set[POKey],
    keys_added: set[POKey],
    original_map: dict[POKey, polib.POEntry],
    normalized_map: dict[POKey, polib.POEntry],
) -> list[tuple[POKey, POKey]]:
  """Find 1:1 unctx→msgctxt upgrades that can stay at the old position."""
  removed_by_msgid: dict[tuple[str, str | None], list[POKey]] = defaultdict(list)
  added_by_msgid: dict[tuple[str, str | None], list[POKey]] = defaultdict(list)

  for key in keys_removed:
    msgctxt, msgid, plural = key
    if msgctxt is None:
      removed_by_msgid[(msgid, plural)].append(key)

  for key in keys_added:
    msgctxt, msgid, plural = key
    if msgctxt is not None:
      added_by_msgid[(msgid, plural)].append(key)

  upgrades: list[tuple[POKey, POKey]] = []
  for msgid_key, old_keys in removed_by_msgid.items():
    new_keys = added_by_msgid.get(msgid_key, [])
    if len(old_keys) != 1 or len(new_keys) != 1:
      continue
    old_key, new_key = old_keys[0], new_keys[0]
    old_entry = original_map[old_key]
    new_entry = normalized_map[new_key]
    if old_entry.msgstr != new_entry.msgstr:
      continue
    if dict(old_entry.msgstr_plural) != dict(new_entry.msgstr_plural):
      continue
    upgrades.append((old_key, new_key))
  return upgrades


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

  if blocks_overlap(original_blocks) or blocks_overlap(normalized_blocks):
    text = normalized_path.read_text(encoding="utf-8")
    original_path.write_text(
        "".join(
            line for line in text.splitlines(True) if not line.startswith("#:")
        ),
        encoding="utf-8",
    )
    return 0

  keys_original = set(original_map)
  keys_normalized = set(normalized_map)

  keys_removed = keys_original - keys_normalized
  keys_added = keys_normalized - keys_original

  keys_changed: set[POKey] = set()
  for key in keys_original & keys_normalized:
    if entry_signature(original_map[key]) != entry_signature(normalized_map[key]):
      keys_changed.add(key)

  upgrades = find_msgctxt_upgrades(
      keys_removed, keys_added, original_map, normalized_map
  )
  for old_key, new_key in upgrades:
    keys_removed.discard(old_key)
    keys_added.discard(new_key)

  replacements: list[tuple[int, int, list[str]]] = []
  for key in keys_removed:
    block = original_blocks.get(key)
    if block is not None:
      replacements.append((block.start, block.end, []))

  for key in keys_changed:
    old_block = original_blocks.get(key)
    new_block = normalized_blocks.get(key)
    if old_block is not None and new_block is not None:
      replacements.append(
          (old_block.start, old_block.end,
           merge_keeping_msgid(old_block.lines, new_block.lines))
      )

  for old_key, new_key in upgrades:
    old_block = original_blocks.get(old_key)
    if old_block is None:
      keys_added.add(new_key)
      continue
    msgctxt = new_key[0]
    assert msgctxt is not None
    replacements.append(
        (old_block.start, old_block.end,
         strip_location_lines(inject_msgctxt(old_block.lines, msgctxt)))
    )

  # Unchanged: drop `#:` only — keep msgid/msgstr wrapping.
  for key in (keys_original & keys_normalized) - keys_changed:
    if key in {ok for ok, _ in upgrades}:
      continue
    block = original_blocks.get(key)
    if block is None:
      continue
    stripped = strip_location_lines(block.lines)
    if stripped != block.lines:
      replacements.append((block.start, block.end, stripped))

  starts = [start for start, _, _ in replacements]
  if len(starts) != len(set(starts)):
    shutil.copyfile(normalized_path, original_path)
    text = original_path.read_text(encoding="utf-8")
    original_path.write_text(
        "".join(line for line in text.splitlines(True) if not line.startswith("#:")),
        encoding="utf-8",
    )
    return 0

  for start, end, new_lines in sorted(replacements, key=lambda x: x[0], reverse=True):
    original_lines[start:end] = new_lines

  if keys_added:
    added_sorted = sorted(
        keys_added, key=lambda key: normalized_map[key].linenum
    )
    append_new_blocks(
        original_lines,
        [
            strip_location_lines(normalized_blocks[key].lines)
            for key in added_sorted
            if key in normalized_blocks
        ],
    )

  # Header `#:` sits outside entry blocks.
  original_lines = strip_location_lines(original_lines)

  with original_path.open("w", encoding="utf-8", newline="") as f:
    f.write("".join(original_lines))
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
