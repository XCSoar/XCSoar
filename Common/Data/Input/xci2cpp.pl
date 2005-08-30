#!/usr/bin/perl
use strict;
use warnings;

my %rec = ();
my $line = 0;
my %labelcount = ();

print "int event_id;\n";
print "int mode_id;\n";
print "\n";
while (<>) {
	chomp;
	$line++;
	next if (/^#/);

	if (/^\s*$/) {
		if ($rec{type}) {
			my $mode = $rec{mode} || die "Invalid entry near $line - no mode\n";

			if ($rec{type} eq "key") {

			} elsif ($rec{type} eq "none") {

			# TODO - Support "nmea"
			} else {
				die "Invaliid record near $line - No valid type: none or key";
			}

			# Make event
			my $event = "&event" . $rec{event};
			die "Invalid event $event near $line" unless ($event =~ /^&event[A-Z].+$/);
			my $misc = $rec{misc} || "";
			print qq{event_id = InputEvents::makeEvent($event, TEXT("$misc"));\n};
			print qq{mode_id = InputEvents::mode2int(TEXT("$mode"));\n};

			# Mode string
			my $label = $rec{label};
			my $location = $rec{location};
			if ($label && $location) {
				my $count = $labelcount{$mode} || 0;
				print qq{ModeLabel[mode_id][$count].label=TEXT("$label");\n};
				print qq{ModeLabel[mode_id][$count].location=$location;\n};
				print qq{ModeLabel[mode_id][$count].event=event_id;\n};
				$labelcount{$mode}++;
			}

			# Key output
			if ($rec{type} eq "key") {
				my $data = $rec{data} || die "Invalid entry near $line - no key\n";
				if (length($data) == 1) {
					$data = qq{TEXT("$data")};
				} else {
					$data = "VK_$data";
				}
				print qq{KeyEvent[mode_id][$data] = event_id;\n};
			}
			print "\n";
		}
		%rec = ();

	# We don't need the quotes - ignore for now
	} elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
		$rec{$1} = $2;

	} else {
		print STDERR "Error on $line - $_\n";
	}

}
