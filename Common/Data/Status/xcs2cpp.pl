#!/usr/bin/perl
use strict;
use warnings;

my %rec = ();
my $line = 0;

=head1 DEMO

	XXX Fix name - don't use Cache !


	StatusMessageCache[StatusMessageCache_Size].key = TEXT("blah");
	StatusMessageCache[StatusMessageCache_Size].sound = TEXT("blah");
	StatusMessageCache[StatusMessageCache_Size].do_sound = true;
	StatusMessageCache[StatusMessageCache_Size].delay_ms = nn;
	StatusMessageCache[StatusMessageCache_Size].doStatus = false;

	StatusMessageCache_Size++;

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
			print q{StatusMessageCache[StatusMessageCache_Size].key = TEXT("} . $rec{key} . qq{");\n};
			if ($rec{sound}) {
				print q{StatusMessageCache[StatusMessageCache_Size].sound = TEXT("}
					. $rec{sound}
					. qq{");\n};
				print qq{StatusMessageCache[StatusMessageCache_Size].doSound = true;\n};
			}
			if ($rec{hide} && ($rec{hide} eq "yes")) {
				print qq{StatusMessageCache[StatusMessageCache_Size].doStatus = false;\n};
			}
			print qq{StatusMessageCache[StatusMessageCache_Size].delay_ms = } . $rec{delay} . qq{;\n};
			print qq{StatusMessageCache_Size++;\n};
		}
		%rec = ();

	} elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
		$rec{$1} = $2;

	} else {
		print STDERR "Error on $line - $_\n";
	}
}
