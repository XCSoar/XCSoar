#!/bin/bash

RAW="${1:-}"
# Accept v7.45.1, 7.45.1, or refs/tags/v7.45.1
VERSION="${RAW##*/}"
VERSION="${VERSION#v}"
STARTLINENUMBER=$(grep -n -E "^Version ${VERSION} " NEWS.txt | cut -f1 -d:)
if [ -z "${STARTLINENUMBER}" ]; then
  echo "ERROR: Version $1 not found in NEWS.txt"
  exit 1
fi
FINLINENUMBER=$(tail -n +"${STARTLINENUMBER}" NEWS.txt | grep -E '^$' -n | head -n 1| cut -f1 -d:)
FINLINENUMBER=$(( "${STARTLINENUMBER}" + "${FINLINENUMBER}" - 2 ))
sed -n "${STARTLINENUMBER}","${FINLINENUMBER}"p NEWS.txt
