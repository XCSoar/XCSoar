#!/usr/bin/perl
use strict;
use warnings;
use Test::More;

my $TESTS = 1;

my @files = qw[
	../Common/Data/Status/default.xcs
	../Common/Data/Status/template.xcs
];

plan tests => (scalar(@files) * $TESTS);

foreach my $file (@files) {
	open (TEST, $file);
	my $line;
	my %rec;

	eval {
		while (<TEST>) {
			chomp;
			$line++;
			next if (/^#/);

			if (/^\s*$/) {
				if ($rec{key}) {
					#print qq{_init_Status(StatusMessageData_Size);\n};
					#print q{StatusMessageData[StatusMessageData_Size].key = TEXT("} . $rec{key} . qq{");\n};
					if ($rec{sound}) {
						#print q{StatusMessageData[StatusMessageData_Size].sound = TEXT("}
							#. $rec{sound}
							#. qq{");\n};
						#print qq{StatusMessageData[StatusMessageData_Size].doSound = true;\n};
					}
					if ($rec{hide} && ($rec{hide} eq "yes")) {
						#print qq{StatusMessageData[StatusMessageData_Size].doStatus = false;\n};
					}
					#print qq{StatusMessageData[StatusMessageData_Size].delay_ms = } . $rec{delay} . qq{;\n};
					#print qq{StatusMessageData_Size++;\n};
				}
				%rec = ();

			} elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
				$rec{$1} = $2;

			} else {
				die "Error on $line - $_";
			}
		}
	};
	if ($@) {
		ok(0, "Failed for $file - $@");
	}
	else {
		ok(1, "Success for $file");
	}
}
