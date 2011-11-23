#!/usr/bin/perl

use strict;
use warnings;
use Data::Dumper;

my @modes = qw(default pan infobox Menu);
my $i = 0;
my %mode_map = map { $_ => $i++ } @modes;

my @events;
my %events2;
my @keys;
my @gestures;
my @gc;
my @nmea;
my @labels;

# Generate a key for the %indices table, to check for duplicates
sub get_key(\%) {
    my $record = shift;

    return unless defined $record->{type};

    my $key = $record->{type};
    $key .= '_' . $record->{mode} if defined $record->{mode};

    if (defined $record->{location}) {
        $key .= '_' . $record->{location};
    } elsif (defined $record->{data}) {
        $key .= '_' . $record->{data};
    }

    return $key;
}

sub append(\@\%\%) {
    my ($all, $indices, $record) = @_;

    my $key = get_key(%$record);
    if (defined $key) {
        if (exists $indices->{$key}) {
            # this entry exists already - overwrite it
            my $i = $indices->{$key};
            $all->[$i] = $record;
            return;
        }

        my $i = scalar @$all;
        $indices->{$key} = $i;
    }

    # append new entry
    push @$all, $record;
}

sub get_mode($) {
    my $name = shift;
    return $mode_map{$name} if exists $mode_map{$name};
    push @modes, $name;
    my $i = $#modes;
    $mode_map{$name} = $i;
    return $i;
}

sub commit(\%) {
    my ($rec) = @_;

    return unless $rec->{type};

    my $line = $rec->{line};
    die unless $line;

    # Make event
    my $event_id = 0;
    foreach my $e (@{$rec->{event}}) {
        my ($handler, $misc) = split(/ /, $e, 2);
        my $e = [ $handler, $misc, $event_id ];
        my $dump = Dumper($e);
        $event_id = $events2{$dump};
        unless (defined $event_id) {
            push @events, $e;
            $event_id = @events;
            $events2{$dump} = $event_id;
        }
    }

    my $mode = $rec->{mode} || die "Invalid entry near $line - no mode\n";
    foreach my $m (split(/ /, $mode)) {
        my $mode_id = get_mode($m);
        my $label = $rec->{label};
        $label =~ s|\\([^rn\\])|\\\\$1|g if ($label);
        $label = '' unless defined $label;
        my $location = $rec->{location};
        push @labels, [ $mode_id, $label, $location, $event_id ]
          if defined $location;

        next unless $event_id > 0;

				# Key output
        if ($rec->{type} eq "key") {
            my $data = $rec->{data} ;
            if (length($data)<1) {
                die "Invalid entry near $line - no key\n";
            }
            if (length($data) == 1) {
                $data = uc($data);
                $data = qq{(int)'$data'};
            } else {
                $data = "VK_$data";
            }

            push @keys, [ $mode_id, $data, $event_id ];
        } elsif ($rec->{type} eq "gce") {
            my $data = $rec->{data} || die "Invalid entry near $line - no GCE data\n";
            push @gc, [ $mode_id, $data, $event_id ];
        } elsif ($rec->{type} eq "ne") {
            my $data = $rec->{data} || die "Invalid entry near $line - no NE data\n";
            push @nmea, [ $mode_id, $data, $event_id ];
        } elsif ($rec->{type} eq "gesture") {
            my $data = $rec->{data} || die "Invalid entry near $line - no Gesture data\n";
            push @gestures, [ $mode_id, $event_id, $data ];
        } elsif ($rec->{type} ne "none") {
            die "Invalid record near $line - No valid type";
        }
    }
}

my @all;
my %indices;

my $current = { event => [] };
my $line = 0;

while (<>) {
    chomp;
    $line++;
    next if (/^#/);

    if (/^\s*$/) {
        append(@all, %indices, %$current);
        $current = { event => [] };

	# We don't need the quotes - ignore for now
    } elsif (/^event\s*=\s*"*([^"]*)"*$/) {
        my $val = $1;
        $val =~ s/\s*$//;
        push @{$current->{event}}, $val;
    } elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
        my $key = $1;
        my $val = $2;
        $val =~ s/\s*$//;
        $current->{$key} = $val;
        $current->{line} = $line unless exists $current->{line};
    } else {
        print STDERR "Error on $line - $_\n";
    }

}

append(@all, %indices, %$current);

foreach my $record (@all) {
    commit(%$record);
}

sub c_string($) {
    my $value = shift;
    return 'NULL' unless defined $value;
    return qq|_T("$value")|;
}

print "static const TCHAR *const default_modes[] = {\n";
foreach my $m (@modes) {
    $m = c_string($m);
    print "  $m,\n";
}
print "  NULL\n";
print "};\n";

print "static const InputConfig::Event default_events[] = {\n";
foreach my $e (@events) {
    my ($handler, $misc, $next_id) = @$e;
    $misc = '' unless defined $misc;
    $misc = c_string($misc);
    print "  { InputEvents::event$handler, $misc, $next_id },\n";
}
print "};\n";

print "static const struct flat_event_map default_key2event[] = {\n";
foreach my $k (@keys) {
    my ($mode, $key, $event) = @$k;
    print "  { $mode, $key, $event },\n";
}
print "  { 0, 0, 0 },\n";
print "};\n";

print "static const struct flat_event_map default_gc2event[] = {\n";
foreach my $k (@gc) {
    my ($mode, $key, $event) = @$k;
    print "  { $mode, GCE_$key, $event },\n";
}
print "  { 0, 0, 0 },\n";
print "};\n";

print "static const struct flat_event_map default_n2event[] = {\n";
foreach my $k (@nmea) {
    my ($mode, $key, $event) = @$k;
    print "  { $mode, $key, $event },\n";
}
print "  { 0, 0, 0 },\n";
print "};\n";

print "static const struct flat_label default_labels[] = {\n";
foreach my $l (@labels) {
    my ($mode, $label, $location, $event) = @$l;
    print qq|  { $mode, $location, $event, _T("$label") },\n|;
}
print "  { 0, 0, 0, NULL },\n";
print "};\n";

print "static const struct flat_gesture_map default_gesture2event[] = {\n";
foreach my $g (@gestures) {
    my ($mode, $event, $data) = @$g;
    print "  { $mode, $event, _T(\"$data\") },\n";
}
print "  { 0, 0, NULL },\n";
print "};\n";
