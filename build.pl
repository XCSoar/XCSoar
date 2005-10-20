#!/usr/bin/perl
use warnings;
use strict;
my $debug = 1;

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
		die "ERROR: Unable to locate $e $p\n\t" . $execs{$e}{$p} . "\n" unless (-e $execs{$e}{$p});
	}
}
# TODO die "ERROR: Unable to locate EZSetup\n\t$exec_ezsetup\n" unless (-e $exec_ezsetup);

# List projects here... (note: we build all of these for each platform)
my @projects = qw/XCSoar XCSoarSimulator XCSoarLaunch/;

# XXX Selection - need to indicate which ones to build from the command line !
my %platforms = (
	'PPC2003' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
	},
	'PPC2002' => {
		'exec' => "EVC3",
		'proc' => [qw/ARM/],
	},
	'PPC' => {	# Also known as PPC 2000
		'exec' => "EVC3",
		'proc' => [qw/ARM/],
		# XXX 'proc' => [qw/ARM MIPS/],
	},
);

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
			#system($cmd) and die("ERROR Executing Command - $?\n\t$cmd\n");
		}
	}
}

# ------------------------------------------------------------------------------
# CABs - via ???
# ------------------------------------------------------------------------------
# XXX Also need to consider command line

foreach my $platform (keys %platforms) {
	my $cmd = q{"} . $execs{$platforms{$platform}{'exec'}}{cabwiz} . q{" }
		. qq{XCSoar$platform.inf /cpu } . join(" ", @{$platforms{$platform}{'proc'}});
	print STDERR "CABing $platform\n";
	print STDERR "\t$cmd\n" if ($debug);
	#system($cmd) and die("ERROR Executing Command - $?\n\t$cmd\n");
}

#"%ProgramFiles%\Windows CE Tools\wce420\POCKET PC 2003\Tools\Cabwiz.exe" XCSoar2002.inf /cpu armv4
#ezsetup -l english -i XCSoar2002.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-ARM2002.exe
#
#"C:\Program Files\Windows CE Tools\wce420\POCKET PC 2003\Tools\Cabwiz.exe" XCSoar.inf /cpu armv4
#ezsetup -l english -i XCSoar.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-ARM2003.exe
#
#"%ProgramFiles%\Windows CE Tools\wce300\MS Pocket PC\support\ActiveSync\windows ce application installation\cabwiz\Cabwiz.exe" XCSoarPPC.inf  /cpu PPC_MIPS PPC_ARM
#ezsetup -l english -i XCSoarPPC.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-PPC.exe
#
#ezsetup -l english -i XCSoarALL.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-ALL.exe
#
#"C:\Program Files\Windows CE Tools\wce420\POCKET PC 2003\Tools\Cabwiz.exe" XCSoarData.inf
#ezsetup -l english -i XCSoarData.ini -r installmsgdata.txt -e gpl.txt -o InstallXCSoar-Data.exe

# ------------------------------------------------------------------------------
# EXEs - via EZSetup
# ------------------------------------------------------------------------------
# XXX Also need to consider command line
# XXX All in one EXE as well
# XXX VERSION Numbers and Rename files

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


