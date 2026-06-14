#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
#
# Reads NEWS.txt and writes a C++ header defining the Quick Guide "What's New"
# body as Markdown: first "# Version …" line is Heading 1, each "* section"
# line is Heading 2, with a blank line before and after each section title,
# "  - " bullets merge indented continuation lines into one list item so the
# in-app renderer wraps with correct hanging indent. A blank line is also
# inserted after the H1.  ``#1234`` issue references become GitHub links.
#
# Quick Guide Markdown constraints (validated before code generation):
# - Use ``* section`` and ``  - `` bullets only; continuations with 4 spaces.
# - Avoid `` `` `` (double backticks); the in-app renderer shows them literally.
# - Avoid ``[text](url)`` examples; MarkdownParser treats them as real links.
# - Do not write ``(#0, #1)`` for USB port suffixes; use ``(0, 1)`` instead.

from __future__ import annotations

import argparse
import re
import sys

# GitHub issue links for NEWS “#1234” references (same tracker as the repo).
GITHUB_ISSUE_BASE = "https://github.com/XCSoar/XCSoar/issues"
# Do not match “#digits” already used as link text in […](…); avoid “##…” headings.
# Skip #0 (not a GitHub issue; used for USB port suffixes in NEWS).
_ISSUE_REF = re.compile(r"(?<!\[)#(?!0\b)(\d+)\b")


def extract_first_version_block(text: str) -> str:
    """Same scope as dlgQuickGuide TruncateToCurrentVersion: first Version line
    through the line before the next top-level ``Version ``."""
    lines = text.split("\n")
    if not lines:
        return ""
    out = [lines[0]]
    for i in range(1, len(lines)):
        if lines[i].startswith("Version "):
            break
        out.append(lines[i])
    return "\n".join(out)


def linkify_github_issue_refs(text: str) -> str:
    """Turn ``#1234`` into ``[#1234](https://github.com/.../issues/1234)``."""

    def repl(m):
        n = m.group(1)
        return f"[#{n}]({GITHUB_ISSUE_BASE}/{n})"

    return _ISSUE_REF.sub(repl, text)


def convert_news_block_to_markdown(block: str) -> str:
    lines = block.split("\n")
    md: list[str] = []
    i = 0
    if i < len(lines):
        first = lines[i].strip()
        if first:
            md.append("# " + first if not first.startswith("# ") else first)
            md.append("")  # blank line after version heading
        i += 1

    while i < len(lines):
        raw = lines[i]
        line = raw.rstrip("\r")

        if line.strip() == "":
            md.append("")
            i += 1
            continue

        stripped = line.lstrip()

        # Section title: single * at line start (NEWS convention), not a bullet.
        if (
            stripped.startswith("* ")
            and not line.startswith("  ")
            and not stripped.startswith("* *")
        ):
            if md and md[-1] != "":
                md.append("")  # blank line before section heading
            md.append("## " + stripped[2:].strip())
            md.append("")  # blank line after section heading
            i += 1
            continue

        # Bullet with optional indented continuations (4 spaces).
        if line.startswith("  - "):
            parts: list[str] = [line[4:].strip()]
            i += 1
            while i < len(lines):
                nl = lines[i]
                if nl.strip() == "":
                    break
                if nl.startswith("    ") and nl[4:].strip():
                    parts.append(nl[4:].strip())
                    i += 1
                    continue
                break
            md.append("- " + " ".join(parts))
            continue

        md.append(line)
        i += 1

    # Trim trailing blank lines; ensure trailing newline
    while md and md[-1] == "":
        md.pop()
    return linkify_github_issue_refs("\n".join(md) + "\n")


def validate_quickguide_markdown(md: str) -> list[str]:
    """Reject constructs that confuse MarkdownParser or mis-link in the UI."""
    errors: list[str] = []
    for i, line in enumerate(md.splitlines(), 1):
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            continue
        if not stripped.startswith("- "):
            errors.append(f"line {i}: expected list item or heading, got: {line[:72]!r}")
            continue
        if "[text](url)" in line:
            errors.append(
                f"line {i}: literal [text](url) is parsed as a Markdown link"
            )
        if "``" in line:
            errors.append(
                f"line {i}: double backticks render literally; use quotes instead"
            )
    return errors


def cxx_escape(s: str) -> str:
    return (
        s.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
    )


def cxx_string_chunks(s: str, chunk: int = 2048) -> str:
    """Adjacent C++ string literals; file is UTF-8.

    Slice the raw input first and escape each chunk independently so that
    escape sequences (\\n, \\", \\\\) are never split across chunk
    boundaries (which would produce invalid C++ literals).
    """
    out: list[str] = []
    for j in range(0, len(s), chunk):
        out.append('"' + cxx_escape(s[j : j + chunk]) + '"')
    return "\n  ".join(out) if out else '""'


def emit_header(markdown: str) -> None:
    chunks = cxx_string_chunks(markdown)
    print("// SPDX-License-Identifier: GPL-2.0-or-later")
    print("// Copyright The XCSoar Project")
    print("")
    print("// Generated by tools/news_to_quickguide_md.py - do not edit.")
    print("")
    print("#pragma once")
    print("")
    print("/** Markdown body for Quick Guide What's New (current version). */")
    print("inline constexpr char quick_guide_news_markdown[] =")
    print(f"  {chunks};")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument(
        "news_file",
        nargs="?",
        default="NEWS.txt",
        help="Path to NEWS.txt (default: NEWS.txt)",
    )
    args = p.parse_args()
    try:
        with open(args.news_file, encoding="utf-8") as f:
            text = f.read()
    except OSError as e:
        print(f"news_to_quickguide_md: {e}", file=sys.stderr)
        return 1

    block = extract_first_version_block(text)
    if not block.strip():
        print("news_to_quickguide_md: empty first version block", file=sys.stderr)
        return 1

    md = convert_news_block_to_markdown(block)
    if not md.strip():
        print("news_to_quickguide_md: conversion produced empty markdown", file=sys.stderr)
        return 1

    for err in validate_quickguide_markdown(md):
        print(f"news_to_quickguide_md: {err}", file=sys.stderr)
        return 1

    emit_header(md)
    return 0


if __name__ == "__main__":
    sys.exit(main())
