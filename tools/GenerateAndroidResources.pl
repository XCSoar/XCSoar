#!/usr/bin/perl -w

use strict;

my %ids;
open IDS, "output/include/resource.h" or die $!;
while (<IDS>) {
    $ids{$1} = $2 if /^#define (\S+) (\d+)/;
}
close IDS;

print <<EOT;
static const struct {
  unsigned id;
  const char *name;
} DrawableNames[] = {
EOT

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^(?:bitmap_bitmap|bitmap_graphic)\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        die unless exists $ids{$1};
        print qq|  { $ids{$1}, "$2" },\n|;
    } elsif (/^(?:app_icon|hatch_bitmap)\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # only used on Windows
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        die unless exists $ids{$1};
        print qq|  { $ids{$1}, "$2" },\n|;

        my $id = $ids{"$1_HD"};
        print qq|  { $id, "$2_160" },\n|;
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # only drawables used here
    } else {
        die "Syntax error: $_";
    }
}

print <<EOT;
  { 0, NULL }
};
EOT
