#!/usr/bin/perl -w

use strict;
use lib './tools';
use BinToC;
use vars qw($as $ar);

sub add_to_src($$) {
    my ($input, $name) = @_;
    BinToC::binary_to_c($input,
                        \*STDOUT,
                        ${name},
                        ${name}."_end",
                        ${name}."_size");
}

while (<>) {
    # merge adjacent strings
    while (s/"([^"]*)"\s+"([^"]*)"\s*$/"$1$2"/) {}

    if (/^\s*(\d+)\s+BITMAP\s+DISCARDABLE\s+"(.*?)"\s*$/ or
          /^\s*([.\w]+)\s+(?:TEXT|XMLDIALOG|MO|RASTERDATA|WAVE)\s+DISCARDABLE\s+"(.*?)"\s*$/) {
        my ($id, $path) = ($1, $2);
        $path = "Data/${path}";
        my $name = "resource_${id}";
        $name =~ s,\.,_,g;

        add_to_src($path, $name);
    }
}
