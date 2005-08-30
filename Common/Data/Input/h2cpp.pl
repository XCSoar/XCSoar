#!/usr/bin/perl
use strict;
use warnings;

open (IN, "../../Header/InputEvents.h");
my $count = 0;
while (<IN>) {
	chomp;

	if (/static void event([a-zA-Z0-9]+)/) {
		print qq{Text2Event[$count].text = TEXT("$1");\n};
		print qq{Text2Event[$count].event = &event$1;\n};
		$count++;
	}
}

print qq{Text2Event_count = $count;\n};

