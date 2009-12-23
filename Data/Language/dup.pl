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
	$data{$_}++;
}

foreach my $key (sort keys %data) {
	print "$key=\n" if ($data{$key} > 1);
}
