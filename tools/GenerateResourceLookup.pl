#!/usr/bin/perl -w
#
# Read the (preprocessed) resources.txt and emit a
# name-to-ResourceId lookup table consumed by ResourceLookup.cpp.

use strict;

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^(?:bitmap_bitmap|bitmap_graphic|hatch_bitmap|app_icon)\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print qq|  { "$1", $1 },\n|;
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print qq|  { "$1", $1 },\n|;
        print qq|  { "${1}_HD", ${1}_HD },\n|;
        print qq|  { "${1}_UHD", ${1}_UHD },\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # sounds are not bitmap resources — skip
    } else {
        die "Syntax error: $_";
    }
}
