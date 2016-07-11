#!/usr/bin/perl -w

use strict;
use lib './tools';
use BinToC;
use vars qw($as $ar);

my $rc_path;
my $output_src_path;

($rc_path, $output_src_path) = @ARGV;

open my $output_src_fh, '>', $output_src_path
  or die "Could not source open output file $output_src_path!\n";

sub add_to_src($$) {
    my ($input, $name) = @_;
    BinToC::binary_to_c($input,
                        $output_src_fh,
                        ${name},
                        ${name}."_end",
                        ${name}."_size");
}

open RC, "<$rc_path" or die $!;

while (<RC>) {
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

close RC;

close $output_src_fh;
