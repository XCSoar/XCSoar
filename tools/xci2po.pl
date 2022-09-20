#!/usr/bin/perl -w

use strict;

my @list;
my %msges;

sub add_message($$$) {
    my ($msg, $filename, $line) = @_;
    my $i = $msges{$msg};
    unless (defined $i) {
        $i = [$msg, ''];
        push @list, $i;
        $msges{$msg} = $i;
    }
    $i->[1] .= "#: $filename:$line\n";
}

while ( <@ARGV> ) {

    open (IN, "< " . $_) or die $!;
    my $filename = $_;

    ## Process one event file like that:
    # mode=pan
    # type=key
    # data=6
    # event=Pan off
    # label=Pan\nOff          -> Pick these labals and dump them in pot format,
    # location=5                 except they are $( ) enclosed
    # ...
    # event=StatusMessage Simulation\r\nNothing is real!
    #                         -> Also pick those messages and dump them into pot format

    my $line = 0;
    while (<>) {
        chomp;
        $line++;

        next if (/^#/);

        if (/^label=([^\$]*?[^\$\s])\s*(?:\\n[^[:alpha:]]*)?(?:\$.*)?$/) {
            my $msg = $1;
            $msg =~ s,\s*\\[nr]$,,g;
            add_message($msg, $filename, $line);
        } elsif (/^event=StatusMessage\s+(\S.*\S)\s*$/) {
            add_message($1, $filename, $line);
        }
    }

    close IN;
}

foreach my $i (@list) {
    my ($k, $v) = @$i;
    print $v . "msgid \"$k\"\nmsgstr \"\"\n\n";
}
