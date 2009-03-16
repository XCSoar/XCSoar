#!/usr/bin/perl
use warnings;
use strict;

my $header = q{

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  $Id$
};

foreach my $file (@ARGV) {
	print STDERR "Fixing copyright in $file\n";
	rename $file, $file . ".tmp";
	open (IN, "< " . $file . ".tmp")
		or die("Unable to open temp file " . $file . ".tmp - you may need to fix by hand - $!");
	my $buffer;
	{
		local $/;
		$buffer = <IN>;
	}
	close IN;

	if ($buffer =~ s/Copyright_License\s*{[^}]*}/Copyright_License {$header}/) {
		print "\tDone\n";
	} else {
		print "\tmissing Copyright_License { } entry\n";
	}

	open (OUT, "> $file")
		or die("Unable to open original input file $file for output - $!");
	print OUT $buffer;
	close OUT;
	unlink($file.".tmp");
}
