#!/usr/bin/env python3

import os, sys

# the path to the XCSoar sources
xcsoar_path = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
sys.path[0] = os.path.join(xcsoar_path, 'build/python')

from build.download import download_and_verify

if len(sys.argv) != 5:
    print("Usage: download.py URL ALTERNATIVE_URL MD5 DIRECTORY", file=sys.stderr)
    sys.exit(1)
    
url, alternative_url, md5, directory = sys.argv[1:]

tarball = os.path.join(directory, os.path.basename(url))

download_and_verify(url, alternative_url, md5, tarball)
