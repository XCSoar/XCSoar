#!/usr/bin/perl -w

use strict;

my $next_id = 100;

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^(?:bitmap_bitmap|bitmap_graphic|hatch_bitmap|app_icon)\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my $id = $next_id++;
        print qq|MAKE_RESOURCE($1, $id);\n|;
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my $id = $next_id++;
        print qq|MAKE_RESOURCE($1, $id);\n|;

        $id = $next_id++;
        print qq|MAKE_RESOURCE($1_HD, $id);\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # not used here
    } else {
        die "Syntax error: $_";
    }
}
