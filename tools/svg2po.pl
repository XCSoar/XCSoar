#!/usr/bin/perl

=head1 SYNOPSIS

B<svg2po.pl> I<inputfiles.svg>

=head1 DESCRIPTION

This program extracts translatable strings from svg files
and prints them to stdout in the gettext PO format.

=cut

use strict;
use warnings;
use vars qw($path %strings);
use IO::CaptureOutput qw/capture_exec/;


my $styl_file = "tools/svg2text.xsl";

sub extract_strings_from_svg($) {
    my $file = shift;
    my ($stdout, $stderr, $success, $exit_code) = capture_exec( "xsltproc --novalid $styl_file $file" );
    if ( ! $success ) {
        print "Your help is needed, cant deal with that error! [$exit_code]\n";
        exit(1);
    }
    my $counter = 1;
    foreach my $line ( $stdout ) {
        chomp $line;
        $strings{$line} ||= [];
        push @{$strings{$line}}, $file . ": " . "$counter";
        $counter += 1;
    }
}

foreach $path (@ARGV) {
    extract_strings_from_svg($path);
}

sub quote($) {
    my $_ = shift;
    s,\\,\\\\,g;
    s,",\\",g;
    s,\n,\\n,g;
    s,\r,\\r,g;
    return $_;
}

foreach my $value (keys %strings) {
    print "#: @{[join(' ', @{$strings{$value}})]}\n";
    print "#, no-c-format\n" if "@{[quote $value]}" =~ m/%/;
    print "msgid \"@{[quote $value]}\"\n";
    print "msgstr \"\"\n";
    print "\n";
}
