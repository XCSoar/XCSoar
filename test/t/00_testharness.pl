#!/usr/bin/perl
use Test::More tests => 1001;
diag("Just testing the test harness");
ok(1, 'Yes it works');

# Example - 1000 entries to show harness
for (1..1000) {
	ok(1, "A loop");
}

