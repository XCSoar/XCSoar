#!/usr/bin/perl
use strict;
use warnings;

my %rec = ();
my $line = 0;

print "/* AUTOMATICALLY GENERATED FILE - DO NOT EDIT BY HAND - see Common/Data/Input/xci2cpp.pl */\n";
print "int event_id;\n";
print "int mode_id;\n";
print "\n";

%rec = ();
$rec{event} = [];
while (<>) {
	chomp;
	$line++;
	next if (/^#/);

	if (/^\s*$/) {
		if ($rec{type}) {
			my $mode = $rec{mode} || die "Invalid entry near $line - no mode\n";
			print "event_id = 0;\n";

			if ($rec{type} eq "key") {

			} elsif ($rec{type} eq "none") {

			} elsif ($rec{type} eq "gce") {
	
			} elsif ($rec{type} eq "ne") {
	
			# TODO - Support "nmea"
			} else {
				die "Invaliid record near $line - No valid type: none or key";
			}

			# Make event
			foreach my $e (@{$rec{event}}) {
				my ($event, $misc) = split(/ /, $e, 2);
				$event = "&event$event";
				die "Invalid event $event near $line" unless ($event =~ /^&event[A-Z].+$/);
				$misc ||= "";
				$misc =~ s|\\([^rn\\])|\\\\$1|g if ($misc);
				print qq{event_id = InputEvents::makeEvent($event, TEXT("$misc"), event_id);\n};
			}

			# TODO Could be faster by grouping these together ! (read file to grouped hash, 
			# then build, one mode2int, less static text in exe file etc.
			foreach my $m (split(/ /, $mode)) {
				print qq{mode_id = InputEvents::mode2int(TEXT("$m"), true);\n};
			
				# Mode string
				my $label = $rec{label};
				$label =~ s|\\([^rn\\])|\\\\$1|g if ($label);
				my $location = $rec{location};
				if ($location) {
					print qq{makeLabel(mode_id,TEXT("$label"),$location,event_id);\n};
				}

				# Key output
				if ($rec{type} eq "key") {
					my $data = $rec{data} ;
					if (length($data)<1) {
					  die "Invalid entry near $line - no key\n";
					}
					if (length($data) == 1) {
						$data = uc($data);
						$data = qq{'$data'};
					} else {
						$data = "VK_$data";
					}
					print qq{Key2Event[mode_id][$data] = event_id;\n};
				} elsif ($rec{type} eq "gce") {
					my $data = $rec{data} || die "Invalid entry near $line - no GCE data\n";
					print qq{GC2Event[mode_id][GCE_$data] = event_id;\n};
				} elsif ($rec{type} eq "ne") {
					my $data = $rec{data} || die "Invalid entry near $line - no NE data\n";
					print qq{N2Event[mode_id][NE_$data] = event_id;\n};
				}
				print "\n";
			}
		}
		%rec = ();
		$rec{event} = [];

	# We don't need the quotes - ignore for now
	} elsif (/^event\s*=\s*"*([^"]*)"*$/) {
		my $val = $1;
		$val =~ s/\s*$//;
		push @{$rec{event}}, $val;
	} elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
		my $key = $1;
		my $val = $2;
		$val =~ s/\s*$//;
		$rec{$key} = $val;
	} else {
		print STDERR "Error on $line - $_\n";
	}

}
