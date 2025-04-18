#!/usr/bin/perl -w

use strict;

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_bitmap\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print "BITMAP_BITMAP($1, \"$2\")\n";
    } elsif (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print "BITMAP_GRAPHIC($1, \"$2\")\n";
    } elsif (/^hatch_bitmap\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print "HATCH_BITMAP($1, \"$2\")\n";
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print "BITMAP_ICON($1, \"$2_96\")\n";
        print "BITMAP_ICON($1_HD, \"$2_160\")\n";
        print "BITMAP_ICON($1_UHD, \"$2_300\")\n";
    } elsif (/^app_icon\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print qq|$1 ICON DISCARDABLE "bitmaps/$2.ico"\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        print "SOUND($1, \"$2\")\n";
    } else {
        die "Syntax error: $_";
    }
}
