#!/usr/bin/perl

use strict;
use warnings;

my %rec = ();
my $line = 0;

sub c_string($) {
    my $value = shift;
    return 'NULL' unless defined $value;
    return qq|_T("$value")|;
}

sub c_bool($) {
    my $value = shift;
    return $value ? "true" : "false";
}

sub print_element($) {
    my $value = shift;
    print "    $value,\n";
}

%rec = ();
while (<>) {
    chomp;
    $line++;
    next if (/^#/);
    s/\r//;

    if (/^\s*$/) {
        if ($rec{key}) {
            print "  {\n";
            print_element(c_string($rec{key}));
            print_element(c_string($rec{sound}));
            print_element(c_bool(not $rec{hide} or $rec{hide} ne "yes"));
            print_element("std::chrono::milliseconds($rec{delay})") if exists $rec{delay};
            print "  },\n",
        }
        %rec = ();

    } elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
        $rec{$1} = $2;
    } else {
        die "Error on line $.\n";
    }
}
