#!/usr/bin/perl

use strict;
use warnings;

use vars qw($path);

use XML::Parser;

my %strings;

sub handle_start {
    my $expat = shift;
    my $element = shift;

    while (scalar @_ >= 2) {
        my ($name, $value) = (shift, shift);

        if ($name eq 'Caption' or $name eq 'Help') {
            next unless $value =~ /[a-zA-Z]/;

            $strings{$value} ||= [];
            push @{$strings{$value}}, $path . ":" . $expat->current_line;
        }
    }
}

my %handlers = (
    Start => \&handle_start,
);

foreach $path (@ARGV) {
    my $parser = new XML::Parser(Handlers => \%handlers);
    $parser->parsefile($path);
}

sub quote($) {
    my $_ = shift;
    s,\\,\\\\,;
    s,",\\",;
    s,\n,\\n,;
    s,\r,\\r,;
    return $_;
}

foreach my $value (keys %strings) {
    print "#: @{[join(' ', @{$strings{$value}})]}\n";
    print "msgid \"@{[quote $value]}\"\n";
    print "msgstr \"\"\n";
    print "\n";
}
