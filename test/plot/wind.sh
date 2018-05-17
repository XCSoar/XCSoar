#!/bin/bash
# author: Max Kellermann <max@xcsoar.org>

if [ -z "$2" -o -n "$3" ]; then
    echo "Usage: $0 DRIVER NMEA" >&2
    cat <<EOF >&2

This script parses a NMEA file and computes the wind vector with all
available methods: EKF, circling and external.  It dumps the results
into text files in *.dat, and plots them using GNUplot.

EOF
    exit 1
fi

./output/UNIX/bin/RunWindEKF "$@" |grep -v '^#' |cut -d' ' -f1,2,3 >ekf.dat
./output/UNIX/bin/RunCirclingWind "$@" |grep -v '^#' |cut -d' ' -f1,3,4 >circling.dat
./output/UNIX/bin/RunExternalWind "$@" |grep -v '^#' |cut -d' ' -f1,2,3 >external.dat
gnuplot `dirname $0`/wind.gnuplot
