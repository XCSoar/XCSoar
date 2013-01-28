#!/usr/bin/perl

############################################################
# $name = "blwindspd";

if ($#ARGV < 1 || $#ARGV > 2) 
{
 print "Usage: ./rasp2jp2.pl <source .data file name> [dest .jp2 file name]\n";
 exit;
}

$dataname = $ARGV[0]; # e.g. blwindspd
$dataname =~ s/.data//g;
$BASEDIR = $ENV{'BASEDIR'};
$infile = "$dataname.data";
$outfile = "$dataname.bin";
$hdrfilename = "$dataname.hdr";
$goutfile = "$dataname.tif";
$basename = `basename $dataname`; chomp $basename;
$jp2outfile = $ARGV[1] ? $ARGV[1] : "$basename.jp2";
$w2 = ($dataname =~ m/\.w2/);

##########################################################

$debug = 1;

# TODO: Get parameters from actual input data file, they're all there

$rfile = $infile;
open(INFILE,"<$rfile");
while(<INFILE>) {
  if (/(.*) \[(.*)] Valid/) {
    $title = $1;
    $units = $2;
    convert_units();
  }
#  lambert 750 750 37.5000 37.5000 -121.5000 37.6213 -121.6874

  if (/.*P\S+= lambert (\S+) \S+ (\S+) (\S+) (\S+) (\S+) (\S+)/) {
    $dx = $1;
    $lat_1 = $2;
    $lat_2 = $3;
    $lon_1 = $4;
    $lat_0 = $5;
    $lon_0 = $6;
    $rat = 1;
  }
}
close(INFILE);

  if(!$w2)
  {
   $lat_2 = $lat_1 = $lat_0;
   $lon_1 = $lon_0;
  }

  print "Title '$title' units $units\n";
  print "dx $dx lambert $lat_1,$lat_2,$lon_1, center $lat_0,$lon_0\n";

open(INFILE,"<$infile");
open(OUTFILE,">$outfile");
binmode OUTFILE;
$ready=0;
while(<INFILE>) {
  if (/Indexs= 1 (\S+) 1 (\S+)/) {
    $rows = $2;
    $cols = $1; # not sure which is which...
  }
  if (/Day/) {
    $ready = 1;
  } else {
    if ($ready) {
      @nums = split(/ /);
      for $item (0..$#nums) {
	my $dat = $nums[$item];
	if ($dat== -999) {
	  $nums[$item]= -32767;
	} else {
	  $dat = ($dat+$unitoffset)*$unitconv+$zerooffset;
	  if ($dat<0) {
	    print "Warning: Negative value! $dat\n";
            $nums[$item] = 0;
	  } else {
	    $nums[$item] = $dat;
	  }
	}
      }
      $bindata[$row] = pack("s$cols",@nums);
      $row++;
    }
  }
};
if ($debug) {
  print "rows $rows cols $cols\n";
}

for ($i=0; $i<$row; $i++) {
  print OUTFILE $bindata[$row-$i-1];
}
close(OUTFILE);
close(INFILE);

#########

$dxr = $dx/$rat;
$ulx = -($dxr*($cols-1)/2);
$uly = ($dxr*($rows-1)/2);
#print "$ulx $uly\n";
open(OUTFILE,">$hdrfilename");
print OUTFILE "BYTEORDER I\n";
print OUTFILE "LAYOUT BIL\n";
print OUTFILE "NBANDS 1\n";
print OUTFILE "NBITS 16\n";
print OUTFILE "NCOLS $cols\n";
print OUTFILE "NROWS $rows\n";
print OUTFILE "ULXMAP $ulx\n";
print OUTFILE "ULYMAP $uly\n";
print OUTFILE "XDIM $dxr\n";
print OUTFILE "YDIM $dxr\n";
close(OUTFILE);

$xmin = $ulx+$dxr;
$xmax = $ulx+$dxr*($cols-1);
$ymin = -$uly+$dxr;
$ymax = -$uly+$dxr*($rows-1);
$xmid = ($xmin+$xmax)/2;
#print "rows $rows cols $cols\n";
#print "$xmin $xmax $ymin $ymax\n";
####################################

#print "ulx $ulx uly $uly xdim $dxr \n";
$x_0=0;
$y_0=0;

$s_proj = "+proj=lcc +lat_1=$lat_1 +lat_2=$lat_2 +lat_0=$lat_0 +lon_0=$lon_0 +x_0=$x_0 +y_0=$y_0 +datum=WGS84";
$t_proj = "+proj=latlong +datum=WGS84";

#TODO: fail if cs2cs doesn't exist

$cmd = "cs2cs $s_proj +to $t_proj -f \"%3.7g %3.7g\"";
if ($debug) {
  print "$cmd\n";
}
open(OUTFILE,"|$cmd > /tmp/res.txt");
print OUTFILE "$xmin $ymin\n";
print OUTFILE "$xmax $ymin\n";
print OUTFILE "$xmin $ymax\n";
print OUTFILE "$xmax $ymax\n";
print OUTFILE "$xmid $ymax\n";
print OUTFILE "$xmid $ymin\n";
close(OUTFILE);
open(INFILE,"</tmp/res.txt");
$i = 0;
while (<INFILE>) {
  if (/(\S+)\s+\S+\s+(\S+)\s+(\S+)/) {
    $lon[$i] = $1;
    $lat[$i] = $2;
#    print "$lon[$i] $lat[$i]\n";
    $i++;
  }
}
close(INFILE);
if ($i==0) {
  print "$xmin $xmax $ymin $ymax\n";
  print "$cmd\n";
  print "Error! Coordinate transformation failed for $dataname\n";
  exit(1);
}
unlink("/tmp/res.txt");

$crop = 1;
if ($crop) {
  if ($lat_0<0) {
    $lat[3] = min($lat[3],$lat[4]);
  } else {
    $lat[1] = max($lat[1],$lat[5]);
  }
  $lat_min = max($lat[0],$lat[1]);
  $lat_max = min($lat[2],$lat[3]);
  $lon_min = max($lon[0],$lon[2]);
  $lon_max = min($lon[1],$lon[3]);
} else {
  $lat_min = $lat_0;
  $lat_max = $lat_0;
  $lon_min = $lon_0;
  $lon_max = $lon_0;
  for ($i=0; $i<6; $i++) {
    $lat_min = min($lat_min,$lat[$i]);
    $lon_min = min($lon_min,$lon[$i]);
    $lat_max = max($lat_max,$lat[$i]);
    $lon_max = max($lon_max,$lon[$i]);
  }
}

#print "lon min max $lon_min $lon_max $lat_min $lat_max\n";

$arg_proj = " -s_srs \"$s_proj\" -t_srs \"$t_proj\" ";
$arg_formats = "-ot Int16 -srcnodata -32767 -dstnodata -32767 -te $lon_min $lat_min $lon_max $lat_max";
$args = " $arg_proj $arg_sizes $arg_formats -q -rcs ";
$cmd = "gdalwarp $args $outfile $goutfile\n";
unlink $goutfile;
if (system($cmd)) {
  print "$lat_min $lat_max $lon_min $lon_max\n";
  print "Error! gdalwarp failed\n";
  print "$cmd\n";
  exit(1);
}
unlink($outfile);
unlink($hdrfilename);

$cmd = "geojasper -f $goutfile -F $jp2outfile -T jp2 -O rate=0.95 -O tilewidth=256 -O tileheight=256 -O xcsoar=1 > /dev/null";
unlink($jp2outfile);
if (system($cmd)) {
  print "Error! geojasper failed\n";
  print "$cmd\n";
  exit(1);
}
unlink($goutfile);

exit(0);

sub max {
  my($a,$b)= @_;
  if ($a>$b) { 
    return $a;
  } else {
    return $b;
  }
}

sub min {
  my($a,$b)= @_;
  if ($a<$b) { 
    return $a;
  } else {
    return $b;
  }
}

sub convert_units {
  $_ = $units;
  $unitoffset = 0;
  $unitconv = 1.0;
  $zerooffset = 0;
  if ($units eq "ft") {
    $unitconv = 0.3048; # ft to m
    return;
  }
  if ($units eq "mAGL") {
    $unitconv = 1.0;
    return;
  }
  if ($units eq "ft AGL - max=18000") {
    $unitconv = 0.3048; # ft to m
    return;
  }
  if ($units eq "\%") {
    return;
  }
  if ($units eq "cm/s") {
    $zerooffset = 200;
    return;
  }
  if ($units eq "W/m~S~2~N~") {
    $zerooffset = 1000;
    return;
  }
  # Deg F = (deg K - 273.15) * 9/5 + 32
  if ($units eq "C") {
    $unitoffset = 20;
    $unitconv = 2.0; # to give range -20 to 50 deg C -> 0 to 140
    $zerooffset = 0;
    return;
  }
  if ($units eq "F") {
    $unitconv = 1.0/1.8;
    $unitoffset = 459.67; # convert to Kelvin
    $zerooffset = -250;
    return;
  }
  if ($units eq "m/s") {
    $unitconv = 100.0; # m/s to cm/s
    $zerooffset = 200;
    return;
  }
  if ($units eq "kt") {
    $unitconv = 51.44; # kt to cm/s
    $zerooffset = 200;
    return;
  }
  if ($units eq "J/kg") {
    $unitconv = 1.0; #
    return;
  }
  if ($units eq "m") {
    $unitconv = 1.0; # ft to m
    return;
  }
  if ($units eq "ft/min") {
    $unitconv = 0.508; # to cm/s
    $zerooffset = 200;
    return;
  }
  print "Unknown unit '$units'\n";
  exit(1);
}

