#!/usr/bin/bash

git fetch --all
# git reset --hard august/weglide-tmp 
git reset --hard flaps3/cmake
make
output/UNIX/bin/xcsoar -fly -1000x500

