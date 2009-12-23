#!/usr/bin/perl
use warnings;
use strict;

while (<>) {
	chomp;
	next if (/^#/);
	s/=$//;
	print "$_=*$_*\r\n";
}
