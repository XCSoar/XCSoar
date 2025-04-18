#!/usr/bin/perl
use strict;
use File::DosGlob qw(bsd_glob);

my $dir = $ARGV[0];
my $waypoint_file = $ARGV[1];

print "IGC directory $dir\n";
print "Waypoint file $waypoint_file\n";

# tar zcofh /tmp/igcbenalla.tgz `grep -li Benalla /var/www/skylines/files/*.igc`

print "Range scan\n";
open(OUTFILE,">output/results/opt_retrospective-range.log");
for (my $range_threshold = 5000; $range_threshold < 30000; $range_threshold*= 1.2) {
    run_all($range_threshold, 25);
}
close(OUTFILE);

print "Angle scan\n";
open(OUTFILE,">output/results/opt_retrospective-angle.log");
for (my $angle_tolerance = 5; $angle_tolerance <= 40; $angle_tolerance+= 5) {
    run_all(10000, $angle_tolerance);
}
close(OUTFILE);

sub run_all {
    my ($range_threshold, $angle_tolerance) = @_;
    my $tot_d_ach = 0;
    my $tot_d_can = 0;
    my $tot_npts = 0;
    my $tot_ratio = 0;
    
    my @igc_files = glob "$dir/*.igc";
    my $n_files = 0;

    print "$range_threshold, $angle_tolerance ";
    
    foreach my $igc_file (@igc_files) {
	my ($d_ach, $d_can, $npts) = run_one($igc_file,$range_threshold,$angle_tolerance);
#    print "$d_ach $d_can $npts\n";
	$tot_d_ach += $d_ach;
	$tot_d_can += $d_can;
	$tot_npts += $npts;
	my $ratio = ($d_can>0)? ($d_ach/$d_can) : 0;
	$tot_ratio += $ratio;
	$n_files++;
	print ".";
    }
    print "\n";

    if ($n_files) {
	$tot_d_ach /= $n_files;
	$tot_d_can /= $n_files;
	$tot_ratio /= $n_files;
	$tot_npts /= $n_files;
	print OUTFILE "$range_threshold $angle_tolerance  ".
	    "$tot_d_ach $tot_d_can $tot_ratio $tot_npts\n";
    }
}

sub run_one {
    my ($igc_file, $range_threshold, $angle_tolerance) = @_;
    my $d_ach = 0;
    my $d_can = 0;
    my $npts = 0;
    my $comd = "output/UNIX/bin/test_replay_retrospective".
	" -d $range_threshold".
	" -n $angle_tolerance".
	" -w $waypoint_file".
	" -f $igc_file";
    open(INFILE,"$comd|");
    while(<INFILE>) {
	if (/distances (\S+) (\S+)/) {
	    $d_ach = $1;
	    $d_can = $2;
	}
	if (/size (\S+)/) {
	    $npts = $1;
	}
    }
    close(INFILE);
    return ($d_ach, $d_can, $npts);
}
