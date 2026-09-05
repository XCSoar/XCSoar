#!/usr/bin/perl -w
#
# Read the (preprocessed) resources.txt and emit a
# name-to-ResourceId lookup table consumed by ResourceLookup.cpp.

use strict;

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print qq|  { "$1", $1 },\n|;
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print qq|  { "$1", $1 },\n|;
        print qq|  { "${1}_MDPI", ${1}_MDPI },\n|;
        print qq|  { "${1}_XHDPI", ${1}_XHDPI },\n|;
        print qq|  { "${1}_XXHDPI", ${1}_XXHDPI },\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # sounds are not bitmap resources — skip
    } else {
        die "Syntax error: $_";
    }
}
