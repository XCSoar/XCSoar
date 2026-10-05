#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Generate one XCSoar software-update repository entry."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import tempfile
from urllib.parse import urlsplit


UINT32_MAX = 2**32 - 1
OFFER_ID_PATTERN = re.compile(r"[A-Za-z0-9._-]{1,63}")
TARGET_PATTERN = re.compile(r"[A-Z0-9_]{1,63}")
DEFAULT_ALLOWED_HOSTS = (
    "xcsoar.org",
    "download.xcsoar.org",
    "apps.apple.com",
)


def validate_version(value: str) -> None:
    if len(value.encode("utf-8")) > 31:
        raise ValueError("version exceeds 31 UTF-8 bytes")

    parts = value.split(".")
    if len(parts) not in (2, 3):
        raise ValueError("version must have two or three numeric components")

    for part in parts:
        if not re.fullmatch(r"[0-9]+", part):
            raise ValueError("version components must be unsigned decimals")
        if int(part) > UINT32_MAX:
            raise ValueError("version component exceeds uint32")


def validate_text(name: str, value: str, maximum: int) -> None:
    if "\n" in value or "\r" in value:
        raise ValueError(f"{name} must be one line")
    if len(value.encode("utf-8")) > maximum:
        raise ValueError(f"{name} exceeds {maximum} UTF-8 bytes")


def validate_url(value: str, allowed_hosts: tuple[str, ...]) -> None:
    if len(value.encode("utf-8")) > 255:
        raise ValueError("url exceeds 255 UTF-8 bytes")
    if not value.startswith("https://"):
        raise ValueError("url must start with https://")

    parsed = urlsplit(value)
    if parsed.scheme != "https" or not parsed.hostname:
        raise ValueError("url must have an HTTPS host")
    if parsed.username is not None or parsed.password is not None:
        raise ValueError("url must not contain user information")

    try:
        port = parsed.port
    except ValueError as error:
        raise ValueError("url has an invalid port") from error

    if port is not None:
        raise ValueError("url must not contain an explicit port")
    if parsed.hostname.lower() not in allowed_hosts:
        raise ValueError("url host is not allow-listed")


def make_entry(*, target: str, version: str, url: str,
               offer_id: str | None = None, summary: str = "",
               source: str = "XCSoar",
               allowed_hosts: tuple[str, ...] = DEFAULT_ALLOWED_HOSTS,
               ) -> str:
    if not TARGET_PATTERN.fullmatch(target):
        raise ValueError("target must contain only A-Z, 0-9, or underscore")

    validate_version(version)
    if offer_id is None:
        offer_id = version
    if not OFFER_ID_PATTERN.fullmatch(offer_id):
        raise ValueError("offer_id contains invalid characters or length")

    validate_text("summary", summary, 383)
    validate_text("source", source, 127)
    validate_url(url, tuple(host.lower() for host in allowed_hosts))

    return (
        f"name = xcsoar-{target}\n"
        f"uri = {url}\n"
        f"description = {summary}\n"
        "type = software-update\n"
        f"target = {target}\n"
        f"version = {version}\n"
        "channel = stable\n"
        f"offer-id = {offer_id}\n"
        f"source = {source}\n"
    )


def write_entry(path: Path, entry: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        "w", encoding="utf-8", dir=path.parent, delete=False
    ) as temporary:
        try:
            temporary.write(entry)
            temporary.flush()
            os.fsync(temporary.fileno())
            temporary_path = Path(temporary.name)
        except Exception:
            Path(temporary.name).unlink(missing_ok=True)
            raise

    try:
        os.replace(temporary_path, path)
    except Exception:
        temporary_path.unlink(missing_ok=True)
        raise


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--url", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--offer-id")
    parser.add_argument("--summary", default="")
    parser.add_argument("--source", default="XCSoar")
    parser.add_argument("--allowed-host", action="append",
                        dest="allowed_hosts")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    allowed_hosts = (
        tuple(args.allowed_hosts)
        if args.allowed_hosts is not None
        else DEFAULT_ALLOWED_HOSTS
    )
    write_entry(args.output, make_entry(
        target=args.target,
        version=args.version,
        url=args.url,
        offer_id=args.offer_id,
        summary=args.summary,
        source=args.source,
        allowed_hosts=allowed_hosts,
    ))


if __name__ == "__main__":
    main()
