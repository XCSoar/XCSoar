#!/usr/bin/perl -w

use strict;

my $next_id = 100;

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my $id = $next_id++;
        print qq|MAKE_RESOURCE($1, $2, $id);\n|;
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        my $id = $next_id++;
        print qq|MAKE_RESOURCE($1, $2_ldpi, $id);\n|;

        $id = $next_id++;
        print qq|MAKE_RESOURCE($1_MDPI, $2_mdpi, $id);\n|;

        $id = $next_id++;
        print qq|MAKE_RESOURCE($1_XHDPI, $2_xhdpi, $id);\n|;

        $id = $next_id++;
        print qq|MAKE_RESOURCE($1_XXHDPI, $2_xxhdpi, $id);\n|;

        # this macro can be passed to MaskedIcon::LoadResource()
        print qq|#define $1_ALL $1, $1_MDPI, $1_XHDPI, $1_XXHDPI\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # not used here
    } else {
        die "Syntax error: $_";
    }
}
