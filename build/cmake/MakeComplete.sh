#!/bin/bash

echo "Make Complete"
echo "============="
git fetch OpenSoaring_OpenSoar
git reset --hard OpenSoaring_OpenSoar/opensoar-dev
./git-clean-submodule.sh
. ./OpenSoar.config
echo "PROGRAM_VERSION = ${PROGRAM_VERSION}"
chmod -R 757 ./build/cmake

export PROGRAM_VERSION=${PROGRAM_VERSION}

if [ "$COMPILE" == "" ]; then COMPILE=y ; fi
if [ "$COMPLETE" == "" ]; then export COMPLETE=y ; fi

if [ "$COMPILE" == "y" ]; then ./build/cmake/MakeAll.sh ; fi

