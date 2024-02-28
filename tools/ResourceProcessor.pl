#!/usr/bin/perl -w

# This script is an ugly kludge to allow macros in XCSoar.rc to emit
# newlines; usually, the C preprocessor cannot emit newlines.  So this
# script calls the preprocessor and then replaces all "NEWLINE"
# strings with a newline character.  Sorry for this mess, we really
# need to replace the whole XCSoar.rc file with something sane.

use strict;

pipe(my $read_fh, my $write_fh) or die "pipe failed: $!";
defined(my $pid = fork) or die "fork failed: $!";

unless ($pid) {
    close $read_fh;
    open STDOUT, '>&', $write_fh or die $!;
    exec @ARGV or die "exec failed: $!";
}

close $write_fh;

while (<$read_fh>) {
    s/\bNEWLINE\b\s*/\n/g;
    print;
}

waitpid($pid, 0);

