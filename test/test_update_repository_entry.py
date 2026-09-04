#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock


SCRIPT = (Path(__file__).parents[1] / "tools" /
          "generate_update_repository_entry.py")
SPEC = importlib.util.spec_from_file_location(
    "generate_update_repository_entry", SCRIPT
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class UpdateRepositoryEntryTest(unittest.TestCase):
    def test_valid_entry(self):
        entry = MODULE.make_entry(
            target="WIN64OPENGL",
            version="7.46.1",
            url="https://download.xcsoar.org/releases/7.46.1/WIN64OPENGL/",
        )
        self.assertIn("type = software-update\n", entry)
        self.assertIn("target = WIN64OPENGL\n", entry)
        self.assertIn("version = 7.46.1\n", entry)
        self.assertIn("offer-id = 7.46.1\n", entry)

    def test_invalid_versions(self):
        for version in ("7", "7.46.1.2", "7.46-rc1", "7..46", "+7.46",
                        "4294967296.1"):
            with self.subTest(version=version), self.assertRaises(ValueError):
                MODULE.make_entry(
                    target="UNIX",
                    version=version,
                    url="https://download.xcsoar.org/releases/7.46/UNIX/",
                )

    def test_rejects_unsafe_urls(self):
        for url in (
            "http://download.xcsoar.org/releases/7.46/UNIX/",
            "HTTPS://download.xcsoar.org/releases/7.46/UNIX/",
            "https://example.org/releases/7.46/UNIX/",
            "https://evil.download.xcsoar.org/releases/7.46/UNIX/",
            "https://download.xcsoar.org:443/releases/7.46/UNIX/",
            "https://user@download.xcsoar.org/releases/7.46/UNIX/",
        ):
            with self.subTest(url=url), self.assertRaises(ValueError):
                MODULE.make_entry(target="UNIX", version="7.46", url=url)

    def test_rejects_multiline_text(self):
        with self.assertRaises(ValueError):
            MODULE.make_entry(
                target="UNIX",
                version="7.46",
                url="https://download.xcsoar.org/releases/7.46/UNIX/",
                summary="one\ntwo",
            )

    def test_atomic_output(self):
        entry = MODULE.make_entry(
            target="UNIX",
            version="7.46",
            url="https://download.xcsoar.org/releases/7.46/UNIX/",
        )
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "UNIX.repository"
            MODULE.write_entry(output, entry)
            self.assertEqual(output.read_text(encoding="utf-8"), entry)

    def test_replacement_failure_removes_temporary_file(self):
        entry = MODULE.make_entry(
            target="UNIX",
            version="7.46",
            url="https://download.xcsoar.org/releases/7.46/UNIX/",
        )
        temporary_paths = []

        def fail_replace(source, destination):
            temporary_paths.append(Path(source))
            raise OSError("simulated replacement failure")

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "UNIX.repository"
            with mock.patch.object(MODULE.os, "replace", fail_replace):
                with self.assertRaises(OSError):
                    MODULE.write_entry(output, entry)

            self.assertEqual(len(temporary_paths), 1)
            self.assertFalse(temporary_paths[0].exists())


if __name__ == "__main__":
    unittest.main()
