#!/bin/sh

rm -f TAGS
etags --declarations -a `find src/ Test/ -type f -name "*.cpp" | grep -v ~`
etags --declarations -a `find src/ Test/ -type f -name "*.hpp" | grep -v ~`
etags --declarations -a `find src/ Test/ -type f -name "*.h" | grep -v ~`
etags --declarations -a `find src/ Test/ -type f -name "*.c" | grep -v ~`
