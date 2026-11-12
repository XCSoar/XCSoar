#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
"""Fetch third-party tarballs into output/download without compiling."""

import os
import re
import sys
from pathlib import Path
from typing import Iterable, Sequence, Tuple, Union

xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

from build.angle import AngleProject
from build.download import download_and_verify, download_basename
from build.verify_sources import iter_projects

UrlList = Union[str, Sequence[str]]
Spec = Tuple[str, UrlList, str]


def _make_vars(path: Path) -> dict:
    vals = {}
    for m in re.finditer(r'^([A-Z][A-Z0-9_]*) = (.*)$',
                         path.read_text(), re.M):
        vals[m.group(1)] = m.group(2).strip()
    return vals


def _boost_specs(root: Path) -> Iterable[Spec]:
    vals = _make_vars(root / 'build/libboost.mk')
    version = vals['BOOST_VERSION']
    underscore = version.replace('.', '_')
    repl = {
        '$(BOOST_VERSION)': version,
        '$(BOOST_UNDERSCORE)': underscore,
    }

    def expand(s: str) -> str:
        for k, v in repl.items():
            s = s.replace(k, v)
        return s

    urls = (expand(vals['BOOST_URL']),
            expand(vals['BOOST_ALTERNATIVE_URL']))
    yield 'boost', urls, vals['BOOST_SHA256']


def _kobo_specs(root: Path) -> Iterable[Spec]:
    vals = _make_vars(root / 'build/kobo.mk')
    for key, url in vals.items():
        if not key.endswith('_URL') or key.endswith('_ALTERNATIVE_URL'):
            continue
        prefix = key[:-4]
        digest = vals.get(prefix + '_SHA256')
        if not digest:
            continue
        alt = vals.get(prefix + '_ALTERNATIVE_URL', url)
        if alt.startswith('$('):
            alt = url
        yield prefix.lower(), (url, alt), digest


def _angle_specs() -> Iterable[Spec]:
    proj = AngleProject()
    for platform, arch in proj.DIGESTS:
        os_platform = 'macos' if platform == 'darwin' else 'windows'
        base = 'angle-%s-%s-%s' % (os_platform, arch, proj.RELEASE_REVISION)
        url = '%s/%s.tar.gz' % (proj.RELEASE_BASE_URL, base)
        name = 'angle-%s-%s' % (os_platform, arch)
        yield name, url, proj.DIGESTS[(platform, arch)]


def _lib_specs() -> Iterable[Spec]:
    seen = set()
    for name, project in iter_projects():
        if isinstance(project, AngleProject):
            continue
        base = download_basename(project.url)
        if base in seen:
            continue
        seen.add(base)
        yield name, project.url, project.md5


def iter_specs(root: Path) -> Iterable[Spec]:
    yield from _lib_specs()
    yield from _angle_specs()
    yield from _boost_specs(root)
    yield from _kobo_specs(root)


def main() -> int:
    dest = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        xcsoar_path, 'output', 'download')
    root = Path(xcsoar_path)
    for name, urls, digest in iter_specs(root):
        print('GET', name)
        download_and_verify(urls, digest, dest)
    return 0


if __name__ == '__main__':
    sys.exit(main())
