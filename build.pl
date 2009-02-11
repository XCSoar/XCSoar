#!/usr/bin/perl
use warnings;
use strict;
my $debug = 0;
my $errors = 0;

my $exec_ezsetup = "ezsetup.exe";

# ------------------------------------------------------------------------------
# Standard Variables
# ------------------------------------------------------------------------------

my $gcc = 0;
my $pf = $ENV{ProgramFiles};
if (!length($pf)) {
  $gcc = 1;
} else {
  if (! -s "$pf/Microsoft eMbedded C++ 4.0/Common/EVC/Bin/EVC.EXE") {
    $pf = "D:/Program Files";
  }
}

my $nobuild = 0;
my $noclean = 1;

my %execs = (
	     'EVC3' => {
			'evc' => "$pf/Microsoft eMbedded C++ 3.0/Common/EVC/Bin/EVC.EXE",
			'cabwiz' => "$pf/Windows CE Tools/wce300/Pocket PC 2002/support/ActiveSync/windows ce application installation/cabwiz/Cabwiz.exe",
		       },
	     'EVC4' => {
			'evc' => "$pf/Microsoft eMbedded C++ 4.0/Common/EVC/Bin/EVC.EXE",
			'cabwiz' => "$pf/Windows CE Tools/wce420/POCKET PC 2003/Tools/Cabwiz.exe",
		       },
	    );

if (!$gcc) {
  foreach my $e (keys %execs) {
    foreach my $p (qw/evc cabwiz/) {
      error("Unable to locate $e $p\n\t" . $execs{$e}{$p} .  "\n") unless (-e $execs{$e}{$p});
    }
  }
  # TODO die "ERROR: Unable to locate EZSetup\n\t$exec_ezsetup\n" unless (-e $exec_ezsetup);
}

# List projects here... (note: we build all of these for each platform)
my @projects = qw/XCSoar XCSoarSimulator XCSoarLaunch XCSoarSetup/;

my %platforms = (
	'PPC2003' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
		'cab' => 1,
		'sim' => 1
	},
	'PNA' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
		'cab' => 0,
		'sim' => 0
	},
	'ALTAIRPORTRAIT' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
		'cab' => 0,
		'sim' => 0
	},
	'ALTAIR' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
		'cab' => 0,
		'sim' => 0
	},
	'PC' => {
		'exec' => "EVC4",
		'proc' => [qw/ARMV4/],
		'cab' => 0,
		'sim' => 1
	},
	'PPC2002' => {
		'exec' => "EVC3",
		'proc' => [qw/ARM/], # was and MIPS
		'cab' => 1,
		'sim' => 1
	},
	# XXX adding MIPS ARM for PPC but using PPC2002 project files !
#	'PPC' => {	# Also known as PPC 2000
#		'exec' => "EVC3",
#		'proc' => [qw/MIPS ARM/],
#	},
);
my @platforms_all = keys %platforms;

# XXX ALL currently not being supported
# push @platforms_all, "ALL";

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
	  if ($user eq "nobuild") {
	    $nobuild = 1;
	  } else {
	    die "ERROR: Invalid platform. Select from:\n\t" . join(",", keys %platforms) .  "\n";
	  }
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
my $version_num = $version;
$version_num =~ s/(\d)\.(\d)\.(\d).*/$1$2$3/g;
print STDERR "Version = ",$version_file, "  num $version_num\n";

# ------------------------------------------------------------------------------
# BUILD ALL via EVC3&4
# ------------------------------------------------------------------------------

if (!$gcc && !$nobuild) {
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
}

# ------------------------------------------------------------------------------
# CABs - via ???
# ------------------------------------------------------------------------------
if (!$gcc) {
  foreach my $platform (keys %platforms) {
    my $cmd = q{"} . $execs{$platforms{$platform}{'exec'}}{cabwiz} . q{" }
      . qq{XCSoar$platform.inf /cpu } . join(" ", @{$platforms{$platform}{'proc'}});
    print STDERR "CABing $platform\n";
    print STDERR "\t$cmd\n" if ($debug);
    system($cmd) and error("Executing Command - $?\n\t$cmd\n");
  }

}

# ------------------------------------------------------------------------------
# EXEs - via EZSetup
# ------------------------------------------------------------------------------

foreach my $platform (@platforms_all) {
  if (!$gcc) {
    my $cmd = q{} . $exec_ezsetup . q{ -l english -i }
      . qq{XCSoar$platform.ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-$platform.exe};
    print STDERR "EZSetup for $platform\n";
    print STDERR "\t$cmd\n" if ($debug);
    system($cmd) and error("Executing Command - $?\n\t$cmd\n");
  } else {
    if (!$nobuild) {
      print "Making cab files with gcc\n";
      system("make -j 2 TARGET=$platform clean");
      if ($platforms{$platform}{cab}) {
	system("make -j 2 TARGET=$platform cab");
      } else {
	system("make -j 2 TARGET=$platform all");
      }
    }
  }
}

# ------------------------------------------------------------------------------
# RENAME for Distribution
# ------------------------------------------------------------------------------
# Rename CAB files

mkdir "dist";
foreach my $platform (keys %platforms) {
  print "Cab files for $platform\n";
  foreach my $proc (@{$platforms{$platform}{'proc'}}) {
    if ($platforms{$platform}{cab}) {
      print "XCSoar$platform.$proc.CAB\n";
      rename "XCSoar$platform.$proc.CAB", "dist/XCSoar$platform.$proc.$version_file.cab"
	or error("Unable to move CAB file $!\n\tXCSoar$platform.$proc.cab\n");
    }
  }
}

# Rename EXE files
foreach my $platform (@platforms_all) {
  if ($platforms{$platform}{cab}) {
    if (!$gcc) {
      rename "InstallXCSoar-$platform.exe", "dist/InstallXCSoar-$platform.$version_file.exe"
	or error("Unable to move EXE file $!\n\tInstallXCSoar-$platform.exe\n");
    } else {

#      rename "XCSoar-$platform.exe", "dist/XCSoar-$platform.$version_file.exe"
#	or error("Unable to move EXE file $!\n\tXCSoar-$platform.exe\n");
#      if ($platforms{$platform}{sim}) {
#	rename "XCSoarSimulator-$platform.exe", "dist/XCSoarSimulator-$platform.$version_file.exe"
#	  or error("Unable to move EXE file $!\n\tXCSoarSimulator-$platform.exe\n");
#      }

    }
  } else {
    if ($platform eq "ALTAIR") {
      rename "XCSoar-$platform.exe","XCSoarAltair-$version_num-CRC3E.exe";
      system("/opt/upx-3.03-amd64_linux/upx XCSoarAltair-$version_num-CRC3E.exe");
      system("cp PPC2003/GRecordDll.dll GRecordDLL.dat");
      system("zip -r XCSoarAltair-$version_file.zip XCSoarAltair-$version_num-CRC3E.exe GRecordDLL.dat");
      rename "XCSoarAltair-$version_file.zip","dist/XCSoarAltair-$version_file.zip";
    } elsif ($platform eq "ALTAIRPORTRAIT") {
      rename "XCSoar-$platform.exe","XCSoarAltair-$version_num-CRC3E.exe";
      system("/opt/upx-3.03-amd64_linux/upx XCSoarAltair-$version_num-CRC3E.exe");
      system("cp PPC2003/GRecordDll.dll GRecordDLL.dat");
      system("zip -r XCSoarAltairPortrait-$version_file.zip XCSoarAltair-$version_num-CRC3E.exe GRecordDLL.dat");
      rename "XCSoarAltairPortrait-$version_file.zip","dist/XCSoarAltairPortrait-$version_file.zip";
    } else {
      if ($platforms{$platform}{sim}) {
	if ($platform eq "PC") {
          system("/opt/upx-3.03-amd64_linux/upx XCSoarSimulator-PC.exe");
          system("/opt/upx-3.03-amd64_linux/upx XCSoar-PC.exe");
	  system("zip -r XCSoar$platform-$version_file.zip XCSoar-PC.exe XCSoarSimulator-PC.exe");
	  rename "XCSoar$platform-$version_file.zip","dist/XCSoar$platform-$version_file.zip";
	} else {
	  rename "XCSoarSimulator-$platform.exe", "XCSoarSimulator.exe"
	    or error("Unable to move EXE file $!\n\tXCSoarSimulator-$platform.exe\n");
	  rename "XCSoar-$platform.exe","XCSoar.exe"
	    or error("Unable to move EXE file $!\n\tXCSoar-$platform.exe\n");
	  system("zip -r XCSoar$platform-$version_file.zip XCSoar.exe XCSoarSimulator.exe");
	  rename "XCSoar$platform-$version_file.zip","dist/XCSoar$platform-$version_file.zip";
	}
      } else {
	rename "XCSoar-$platform.exe","XCSoar.exe"
	  or error("Unable to move EXE file $!\n\tXCSoar-$platform.exe\n");
	system("zip -r XCSoar$platform-$version_file.zip XCSoar.exe");
	rename "XCSoar$platform-$version_file.zip","dist/XCSoar$platform-$version_file.zip";
      }
    }
  }
}

# --------------
# Clean after build

if (!$gcc && !$noclean) {
  foreach my $platform (keys %platforms) {
    foreach my $project (@projects) {
      foreach my $proc (@{$platforms{$platform}{'proc'}}) {
	my $cmd = q{"} . $execs{$platforms{$platform}{'exec'}}{evc} . q{" }
	  . qq{$platform/$project/$project.vcp /MAKE "$project - Win32 (WCE $proc) Release" /CLEAN};
	print STDERR "Clean $project for $platform/$proc\n";
	print STDERR "\t$cmd\n" if ($debug);
	system($cmd) and error("Executing Command - $?\n\t$cmd\n");
      }
    }
  }
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


