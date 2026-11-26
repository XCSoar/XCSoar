#!/usr/bin/env python3

import os, sys
from pathlib import Path

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

from build.download import download_and_verify, download_basename

if len(sys.argv) != 5:
    print("Usage: download.py URL ALTERNATIVE_URL MD5 DIRECTORY", file=sys.stderr)
    sys.exit(1)

urls = sys.argv[1:3]
# OBS places remote assets alongside the build recipe.
asset = Path(xcsoar_path) / download_basename(urls)
if asset.is_file():
    urls = [asset.as_uri()]

download_and_verify(urls, sys.argv[3], sys.argv[4])
