#!/usr/bin/perl

use strict;

my $line = 0;
my $filename = "";
my %msges = ();

while ( <@ARGV> ) {

    open (IN, "< " . $_) or die $!;
    $filename = $_;

    ## Process one event file like that:
    # mode=pan
    # type=key
    # data=6
    # event=Pan supertoggle
    # label=Pan\nOff          -> Pick these labals and dump them in pot format,
    # location=5                 except they are $( ) enclosed

    $line = 0;
    while (<>) {
        chomp;
        $line++;

        next if (/^#/);

        if ( /^label=([^\$]+)/ ) {
            $msges{ $1 } .= "#: $filename:$line\n";
        }
    }

    close IN;
}

my $k = "";
my $v = "";

while (($k, $v) = each %msges) {
    print $v . "msgid \"$k\"\nmsgstr \"\"\n\n";
}
