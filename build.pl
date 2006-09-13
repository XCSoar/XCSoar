#!/usr/bin/perl
use warnings;
use strict;
my $debug = 0;
my $errors = 0;

# ------------------------------------------------------------------------------
# Standard Variables
# ------------------------------------------------------------------------------
my $pf = $ENV{ProgramFiles};
my %execs = (
	'EVC3' => {
		'evc' => "$pf\\Microsoft eMbedded Tools\\Common\\EVC\\Bin\\EVC.EXE",
		'cabwiz' => "$pf\\Windows CE Tools\\wce300\\MS Pocket PC\\support\\ActiveSync\\windows ce application installation\\cabwiz\\Cabwiz.exe",
	},
	'EVC4' => {
		'evc' => "$pf\\Microsoft eMbedded C++ 4.0\\Common\\EVC\\Bin\\EVC.EXE",
		'cabwiz' => "$pf\\Windows CE Tools\\wce420\\POCKET PC 2003\\Tools\\Cabwiz.exe",
	},
);
my $exec_ezsetup = "ezsetup.exe";
foreach my $e (keys %execs) {
	foreach my $p (qw/evc cabwiz/) {
		error("Unable to locate $e $p\n\t" . $execs{$e}{$p} .  "\n") unless (-e $execs{$e}{$p});
	}
}
# TODO die "ERROR: Unable to locate EZSetup\n\t$exec_ezsetup\n" unless (-e $exec_ezsetup);

# List projects here... (note: we build all of these for each platform)
my @projects = qw/XCSoar XCSoarSimulator XCSoarLaunch XCSoarSetup/;
my @projects = qw/XCSoarSimulator/;

my %platforms = (
	'PPC2003' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
	},
	'PPC2002' => {
		'exec' => "EVC3",
		'proc' => [qw/ARM MIPS/],
	},
	# XXX adding MIPS ARM for PPC but using PPC2002 project files !
#	'PPC' => {	# Also known as PPC 2000
#		'exec' => "EVC3",
#		'proc' => [qw/MIPS ARM/],
#	},
);
my @platforms_all = keys %platforms;
push @platforms_all, "ALL";

# ------------------------------------------------------------------------------
# User Input (primative)
# ------------------------------------------------------------------------------
my $user = shift;
if ($user) {
	if (exists($platforms{$user}) || $user eq "ALL") {
		foreach my $key (keys %platforms) {
			delete $platforms{$key} unless ($key eq $user);
		}
		@platforms_all = ($user);
	} else {
		die "ERROR: Invalid platform. Select from:\n\t" . join(",", keys %platforms) .  "\n";
	}
}
print STDERR "Building platforms: " . join(",", keys %platforms) . "\n";

# ------------------------------------------------------------------------------
# Process the VERSION
# ------------------------------------------------------------------------------
open (IN, "VERSION.txt") || die "ERROR: Unable to open version file: VERSION.txt - $!\n";
my $version_raw = <IN>;
close IN;
chomp($version_raw);
# __DATE__ = "mmm dd yyyy"
my $version = $version_raw;
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$year+=1900; 
my $month = qw/Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec/[$mon];
$version =~ s/__DATE__/$month $mday $year/g;
my $version_file = $version;
$version_file =~ s/ /_/g;

# ------------------------------------------------------------------------------
# BUILD ALL via EVC3&4
# ------------------------------------------------------------------------------
foreach my $platform (keys %platforms) {
	foreach my $project (@projects) {
		foreach my $proc (@{$platforms{$platform}{'proc'}}) {
			my $cmd = q{"} . $execs{$platforms{$platform}{'exec'}}{evc} . q{" }
				. qq{$platform/$project/$project.vcp /MAKE "$project - Win32 (WCE $proc) Release" /REBUILD};
			print STDERR "Building $project for $platform/$proc\n";
			print STDERR "\t$cmd\n" if ($debug);
			system($cmd) and error("Executing Command - $?\n\t$cmd\n");
		}
	}
}

# ------------------------------------------------------------------------------
# CABs - via ???
# ------------------------------------------------------------------------------
foreach my $platform (keys %platforms) {
	my $cmd = q{"} . $execs{$platforms{$platform}{'exec'}}{cabwiz} . q{" }
		. qq{XCSoar$platform.inf /cpu } . join(" ", @{$platforms{$platform}{'proc'}});
	print STDERR "CABing $platform\n";
	print STDERR "\t$cmd\n" if ($debug);
	system($cmd) and error("Executing Command - $?\n\t$cmd\n");
}

# ------------------------------------------------------------------------------
# EXEs - via EZSetup
# ------------------------------------------------------------------------------
foreach my $platform (@platforms_all) {
	my $cmd = q{} . $exec_ezsetup . q{ -l english -i }
		. qq{XCSoar$platform.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-$platform.exe};
	print STDERR "EZSetup for $platform\n";
	print STDERR "\t$cmd\n" if ($debug);
	system($cmd) and error("Executing Command - $?\n\t$cmd\n");
}

# ------------------------------------------------------------------------------
# RENAME for Distribution
# ------------------------------------------------------------------------------
# Rename CAB files
mkdir "dist";
foreach my $platform (keys %platforms) {
	foreach my $proc (@{$platforms{$platform}{'proc'}}) {
		rename "XCSoar$platform.$proc.cab", "dist\\XCSoar$platform.$proc.$version_file.cab"
			or error("Unable to move CAB file $!\n\tXCSoar$platform.$proc.cab\n");
	}
}

# Rename EXE files
foreach my $platform (@platforms_all) {
	rename "InstallXCSoar-$platform.exe", "dist\\InstallXCSoar-$platform.$version_file.exe"
		or error("Unable to move EXE file $!\n\tInstallXCSoar-$platform.exe\n");
}

# ------------------------------------------------------------------------------
# Upload and Announce
# ------------------------------------------------------------------------------
# TODO - Upload to Sourceforge (optional)
# TODO - Announce to lists & Freshmeat etc


print STDERR "PROCESSING COMPLETE: $errors errors\n";

exit 0;

sub error {
	my ($msg) = @_;
	print STDERR "ERROR: $msg\n";
	print STDERR "Continue (Y/N)? ";
	my $A = <STDIN>;
	unless (uc(substr($A, 0, 1)) eq "Y") {
		print STDERR "PROCESSING INCOMPLETE!\n";
		exit 1;
	}
	$errors++;
}
__END__

=====
NOTES
=====

EVC Help

Usage:
  EVC [myprj.vcp|mywksp.vcw]    - load project/workspace
        [<filename>]            - load source file
        /?                      - display usage information
        /EX <macroname>         - execute a VBScript macro
        /OUT <filename>         - redirect command line output to a file
        /USEENV                 - ignore tools.options.directories settings
        /MAKE [<target>] [...]  - build specified target(s)
              [<project> - <platform> <configname>]
              [[<project>|ALL] - [DEBUG|RELEASE|ALL]]
              /CLEAN            - delete intermediate files but don't build
              /REBUILD          - clean and build
              /NORECURSE        - don't build dependent projects
              /CECONFIG [<configuration>]       - use specified configuration


