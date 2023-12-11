#!/bin/bash

echo "Make Complete"
echo "============="

if [ "$GIT_REPOSITORY" == "" ]; then GIT_REPOSITORY=OpenSoaring ; fi
if [ "$GIT_BRANCH" == "" ]; then GIT_BRANCH=master ; fi
#  or origin ; fi

git fetch ${GIT_REPOSITORY}
git reset --hard ${GIT_REPOSITORY}/${GIT_BRANCH}
./git-clean-submodule.sh
. ./OpenSoar.config
echo "PROGRAM_VERSION = ${PROGRAM_VERSION}"
chmod -R 757 ./build/cmake

export PROGRAM_VERSION=${PROGRAM_VERSION}

if [ "$COMPILE" == "" ]; then COMPILE=y ; fi
if [ "$COMPLETE" == "" ]; then export COMPLETE=y ; fi

if [ "$COMPILE" == "y" ]; then ./build/cmake/MakeAll.sh ; fi

