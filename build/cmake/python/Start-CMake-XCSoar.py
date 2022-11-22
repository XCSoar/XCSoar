#!/usr/bin/env python3

import os, sys
from CMakeXCSoar import create_xcsoar

# ====================
# ====================
branch = 'dev-branch'
# ====================
# ====================


class ComputerDirectories(object):
    def __init__(self, name, directories):
        self.name = name
        self.project_dir = directories["project_dir"]
        self.third_party_dir = directories["third_party_dir"]
        self.binary_dir = directories["binary_dir"]
        self.program_dir = directories["program_dir"]
        self.link_libs = directories["link_libs"]

creation_flag = 15
if len(sys.argv) > 1:
  project = sys.argv[1]
else:
  project = "XCSoarAug"

if len(sys.argv) > 2:
  toolchain = sys.argv[2]
else:
  toolchain = "msvc2022"
  
if len(sys.argv) > 3:
  creation_flag = sys.argv[3]
  
# input("Press Enter to continue...") # == 'pause'


if sys.platform.startswith('win'):
    if not toolchain in ['mgw73', 'mgw103', 'mgw112', 'mgw122', 'ninja', 'msvc2019', 'msvc2022',
        'clang10', 'clang11', 'clang12', 'clang13', 'clang14', 'clang15' ]:
        toolchain = 'mgw112'  # standard toolchain on windows
else:
    if not toolchain in ['unix', 'mingw']:
        # toolchain = 'unix'  # standard toolchain on Linux
        toolchain = 'mingw'  # standard toolchain on Linux

print('Project Name = ', project, 'toolchain = ', toolchain)

arguments = []
arguments.append('XCSoarAug')   # project_name
# arguments.append('master')      # branch
# arguments.append('weglide_msvc_new')      # branch
if branch:
  arguments.append(branch)      # branch
else:
  arguments.append('no-branch')      # branch

arguments.append(toolchain)     # build-toolchain
arguments.append(creation_flag)   
# arguments.append('Arg2')


# arguments.append('Arg3')
# arguments.append('Arg4')

print('Jetzt gehts los: ', arguments)
create_xcsoar(arguments)

