#!/usr/bin/perl
#
# Send a NMEA dump to the Android emulator.
#
# Example: AndroidMockLocation.pl test.nmea 127.0.0.1:5554
#

use strict;
use warnings;
require IO::Socket;

die "Usage: AndroidMockLocation.pl FILE.nmea HOST:PORT\n"
  unless @ARGV == 2;

my ($path, $address) = @ARGV;

open FILE, "<$path" or die "Failed to open $path: $!\n";

my $socket = new IO::Socket::INET(PeerAddr => $address);
die "Failed to connect to emulator: $!\n" unless defined $socket;

$socket->getline;
$socket->getline;

my $time = '';
while (<FILE>) {
    if (/^\$..GGA,(\d+)/) {
        my $new_time = $1;
        unless ($new_time eq $time) {
            # sleep for a second when the time stamp has been updated
            sleep 1;
            $time = $new_time;
        }
    } elsif (/^\$..RMC,/) {
    } else {
        # the Android emulator understands only GPGGA and GPRMC
        next;
    }

    my $line = "geo nmea $_";
    print $line;
    $socket->syswrite($line) or die $!;
    my $response = $socket->getline;
    die $response unless $response =~ /^OK/;
}
