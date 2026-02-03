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
		# Run Windows .exe under Wine when not on Windows
		if ($test_file =~ /\.exe$/i && $^O ne 'MSWin32') {
			return [ 'wine', $test_file ] if -f $test_file;
		} elsif (-x $test_file) {
			return [ $test_file ];
		}
		die "Unknown test file type - $test_file\n";
	},
});
my $aggregator = $harness->runtests(@ARGV);
exit($aggregator->exit);