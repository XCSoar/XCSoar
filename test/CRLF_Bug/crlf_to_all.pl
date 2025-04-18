#!/usr/bin/perl
use warnings;
use strict;

foreach my $ext (@ARGV) {

	my $input;
	open ($input, "original.$ext");
	my @lines = ();
	while (<$input>) {
		chomp;
		s/\r//g;
		s/\n//g;
		push @lines, $_;
	}
	close $input;


	output("crlf.$ext", "\r\n", \@lines);	# Standard windows
	output("lf.$ext", "\n", \@lines);	# Standard unix
	output("cr.$ext", "\r", \@lines);	# Standard Mac
	output("lfcr.$ext", "\n\r", \@lines);	# Broken
	output("crcrlf.$ext", "\r\r\n", \@lines);# Very broken
}
exit 0;

sub output {
	my ($filename, $delim, $lines) = @_;
	my $fileref;
	open ($fileref, "> $filename");
	foreach my $line (@$lines) {
		print $fileref $line . $delim;
	}
	close $fileref;
}
