#!/bin/bash

git fetch OpenSoaring_OpenSoar
# git reset --hard august/weglide-tmp 
# git reset --hard flaps3/cmake
git reset --hard OpenSoaring_OpenSoar/opensoar-dev
# git reset --hard OpenSoaring_OpenSoar/dev-branch
sudo chmod 757 -R ./build/cmake
make TARGET=UNIX DEBUG=n
output/UNIX/bin/xcsoar -fly -1000x500

