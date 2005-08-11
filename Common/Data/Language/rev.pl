#!/usr/bin/perl
use warnings;
use strict;

#
# Find entries in template that are missing from other files
#

my %data = ();
while (<>) {
	chomp;
	next if (/^#/);
	s/=.*$//;
	$data{$_} = 0;
}

open (T, "template.xcl");
while (<T>) {
	chomp;
	next if (/^#/);
	s/=.*$//;
	$data{$_} = 1;
}

foreach my $key (sort keys %data) {
	print "$key=\n" if ($data{$key} == 0);
}
