#!/usr/bin/python
#
# This program installs the resources in ~/.xcsoar/, to make XCSoar
# work on UNIX platforms.  This is a hack to allow loading the
# resources by their ID number.
#

import re, os, shutil

resource_re = re.compile(r'^(ID[BR]_\S+)\s+(\S+)\s+DISCARDABLE\s+"([^"]+)"\s*$')
macro_re = re.compile(r'^#define\s+(ID[BR]_\S+)\s+(\d+)\s*$')

resources = {}
for line in file('Data/XCSoar.rc'):
    m = resource_re.match(line)
    if m is None: continue
    name, path = m.group(1), m.group(3)
    resources[name] = path

dest = os.path.join(os.environ['HOME'], '.xcsoar', 'resources')
try:
    os.makedirs(dest)
except OSError:
    pass

for line in file('src/resource.h'):
    m = macro_re.match(line)
    if m is None: continue
    name, id = m.group(1), m.group(2)
    if not name in resources: continue
    src_path = os.path.join('Data', resources[name])
    dest_path = os.path.join(dest, id)
    shutil.copy(src_path, dest_path)
