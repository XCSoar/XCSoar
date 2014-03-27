#!/usr/bin/perl -w

use strict;
use lib './tools';
use EmbedFileInObjectFile;
use vars qw($as $ar);

my $rc_path;
my $output_path;

($rc_path, $output_path, $as, $ar) = @ARGV;
$output_path =~ m,^(.*)/,s;
my $o_path = "$1/resources";

sub to_o($$$) {
    my ($input, $output, $name) = @_;

    EmbedFileInObjectFile::embed_file_in_object_file($input, $output,
            ${name}, ${name}."_end", ${name}."_size", $as);
}

open RC, "<$rc_path" or die $!;

my @objects;

while (<RC>) {
    # merge adjacent strings
    while (s/"([^"]*)"\s+"([^"]*)"\s*$/"$1$2"/) {}

    if (/^\s*(\d+)\s+BITMAP\s+DISCARDABLE\s+"(.*?)"\s*$/ or
          /^\s*([.\w]+)\s+(?:TEXT|XMLDIALOG|MO|RASTERDATA)\s+DISCARDABLE\s+"(.*?)"\s*$/) {
        my ($id, $path) = ($1, $2);
        $path = "Data/${path}";
        my $name = "resource_${id}";
        $name =~ s,\.,_,g;

        my $output = "${o_path}/${name}.o";
        to_o($path, $output, $name);
        push @objects, $output;
    }
}

close RC;

system("${ar} ${output_path} " . join(' ', @objects)) == 0 or die;
