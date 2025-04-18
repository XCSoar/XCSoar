#!/usr/bin/perl -w

use strict;
use lib './tools';
use BinToC;

sub add_to_src($$) {
    my ($input, $name) = @_;
    BinToC::binary_to_c($input,
                        \*STDOUT,
                        ${name},
                        ${name}."_end",
                        ${name}."_size");
}

my $enable_opengl = $ENV{ENABLE_OPENGL} eq 'y';
my $target_is_android = $ENV{TARGET_IS_ANDROID} eq 'y';

my $icons_dir = $enable_opengl
  ? 'output/data/icons'
  : 'output/data/icons2';

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^bitmap_bitmap\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        add_to_src("output/data/bitmaps/$2.png", "resource_$1");
    } elsif (/^bitmap_graphic\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        add_to_src("output/data/graphics2/$2.png", "resource_$1");
    } elsif (/^(?:app_icon|hatch_bitmap)\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # only used on Windows
    } elsif (/^bitmap_icon_scaled\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        add_to_src("$icons_dir/$2_96.png", "resource_$1");
        add_to_src("$icons_dir/$2_160.png", "resource_$1_HD");
        add_to_src("$icons_dir/$2_300.png", "resource_$1_UHD");
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        add_to_src("output/data/sound/$2.raw", "resource_$1") unless $target_is_android;
    } else {
        die "Syntax error: $_";
    }
}
