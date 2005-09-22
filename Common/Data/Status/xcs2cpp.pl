#!/usr/bin/perl
use strict;
use warnings;

my %rec = ();
my $line = 0;

=head1 DEMO

	StatusMessageData[StatusMessageData_Size].key = TEXT("blah");
	StatusMessageData[StatusMessageData_Size].sound = TEXT("blah");
	StatusMessageData[StatusMessageData_Size].do_sound = true;
	StatusMessageData[StatusMessageData_Size].delay_ms = nn;
	StatusMessageData[StatusMessageData_Size].doStatus = false;

	StatusMessageData_Size++;

=cut

print "/* AUTOMATICALLY GENERATED FILE - DO NOT EDIT BY HAND - see Common/Data/Status/xcs2cpp.pl */\n";
print "\n";

%rec = ();
while (<>) {
	chomp;
	$line++;
	next if (/^#/);

	if (/^\s*$/) {
		if ($rec{key}) {
			print qq{_init_Status(StatusMessageData_Size);\n};
			print q{StatusMessageData[StatusMessageData_Size].key = TEXT("} . $rec{key} . qq{");\n};
			if ($rec{sound}) {
				print q{StatusMessageData[StatusMessageData_Size].sound = TEXT("}
					. $rec{sound}
					. qq{");\n};
				print qq{StatusMessageData[StatusMessageData_Size].doSound = true;\n};
			}
			if ($rec{hide} && ($rec{hide} eq "yes")) {
				print qq{StatusMessageData[StatusMessageData_Size].doStatus = false;\n};
			}
			print qq{StatusMessageData[StatusMessageData_Size].delay_ms = } . $rec{delay} . qq{;\n};
			print qq{StatusMessageData_Size++;\n};
		}
		%rec = ();

	} elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
		$rec{$1} = $2;

	} else {
		print STDERR "Error on $line - $_\n";
	}
}
