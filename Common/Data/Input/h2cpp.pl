#!/usr/bin/perl
use strict;
use warnings;

print "/* AUTOMATICALLY GENERATED FILE - DO NOT EDIT BY HAND - see Common/Data/Input/h2cpp.pl */\n";
open (IN, "../../Header/InputEvents.h");
my $count = 0;

my $buffer = "";
my $gce_count = 0;
my $ne_count = 0;
while (<IN>) {
	chomp;
	
	if (/static void event([a-zA-Z0-9]+)/) {
		print qq{Text2Event[$count].text = TEXT("$1");\n};
		print qq{Text2Event[$count].event = &event$1;\n};
		$count++;
	} elsif (/GCE_([A-Z0-9_]+)/) {
		print qq{Text2GCE[$gce_count] = TEXT("$1");\n};
		$gce_count++;
	} elsif (/NE_([A-Z0-9_]+)/) {
		print qq{Text2NE[$ne_count] = TEXT("$1");\n};
		$ne_count++;
	}
}

print qq{Text2Event_count = $count;\n};

$ne_count--;
$gce_count--;
print qq{// Check no array overrun\nASSERT($ne_count <= NE_COUNT);\n};
print qq{// Check no array overrun\nASSERT($gce_count <= GCE_COUNT);\n};

