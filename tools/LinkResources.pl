#!/usr/bin/perl -w

use strict;
use vars qw($ld $ar);

my $rc_path;
my $output_path;

($rc_path, $output_path, $ld, $ar) = @ARGV;
$output_path =~ m,^(.*)/,s;
my $o_path = "$1/resources";

sub to_o($$$) {
    my ($input, $output, $name) = @_;

    my $name2 = "_binary_" . $input;
    $name2 =~ s,[^_a-zA-Z0-9],_,sg;

    system("${ld} -r -b binary -o ${output} ${input}"
             . " --defsym=${name}=${name2}_start"
               . " --defsym ${name}_end=${name2}_end"
                 . " --defsym ${name}_size=${name2}_size"
              ) == 0 or die;
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
