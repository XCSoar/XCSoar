#!/usr/bin/perl
use warnings;
use strict;
use TAP::Harness;
my $harness = TAP::Harness->new({
        timer => 1,
        color => 1,
	exec => sub {
		my ($harness, $test_file) = @_;
		# Let Perl tests run.
		return undef if $test_file =~ /[.].pl$/;
		if (-x $test_file) {
			return [ $test_file ];
		}
		die "Unknown test file type - $test_file\n";
	},
});
$harness->runtests(@ARGV);
