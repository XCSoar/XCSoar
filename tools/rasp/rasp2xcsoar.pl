#!/usr/bin/perl

print "Usage: ./rasp2xcsoar.pl [-w2] [path]\n";
print "  [-w2] use this option to process .w2. data files from the windowed run.\n";
print "  [path] local path to the directory containing .data files\n";
print "         or http link to the web folder containing .data.zip files.\n\n";

$BASEDIR = $ENV{'BASEDIR'};
$d2w2 = ($ARGV[0] eq "-w2") ? "w2" : "d2";
$indir = $d2w2 ? $ARGV[1] : $ARGV[0];
if ($indir eq "") {
  $indir = "$BASEDIR/RASP/HTML/BYRON/FCST/";
  print "Trying to use the default path: $indir\n";
};
$_= $indir;
if (/http/) {
  $download = 1;
} else {
  $download = 0;
}


@itemlist = ( 
	     "sfctemp",
	     "blwindspd",
	     "blwindshear",
	     "blcwbase",
	     "blcloudpct",
	     "dwcrit",
	     "hbl",
	     "hwcrit",
	     "wstar",
	     "zsfclcl",
	     "wblmaxmin"
#	     "cape",
#	     "dbl",
#	     "sfcdewpt",
#	     "sfcshf",
#	     "sfcsun",
#	     "sfcwindspd",
);

$zipfile = "xcsoar-rasp.zip";
unlink($zipfile);

foreach $item (0..$#itemlist) {
 foreach $minute(0..5) {
  foreach $time (4..22) {
    if ($time<10) {
      $otime = "0${time}${minute}0";
    } else {
      $otime = "${time}${minute}0";
    }
    $fname = "$itemlist[$item].curr.${otime}lst.$d2w2.data";
    if ($download) {

      system("rm $fname");
      $comd = "wget -q -O $fname.zip $indir/$fname.zip";
      print "$comd\n";
      system($comd) && ($ok=0);
      system("unzip -oqq $fname.zip") && ($ok=0);
      if (-r $fname) {
	$ok = 1;
      } else {
	system("rm $fname.zip");
	$ok = 0;
      }
      if (!$ok) {
	$comd = "wget -q $indir/$fname";
	print "$comd\n";
	system($comd) && ($ok=0);
	if (-r $fname) {
	  $ok = 1;
	} else {
	  $ok = 0;
	}
      }
      $infile = "$fname";
      if ($ok) {
	print "$infile\n";
      }
    } else {
    $infile = "$indir/$itemlist[$item].curr.${otime}lst.$d2w2.data";
      $ok = 1;
    }

    $jp2file = "$itemlist[$item].curr.${otime}lst.d2.jp2";
    $zinfile = "$infile.zip";
    if ($ok && (-r $zinfile)) {
      $comd = "unzip -ocq $zinfile > $infile";
      system($comd);
      $is_zip = 1;
    } else {
      $is_zip = 0;
    }
    if (-r $infile) {
      print "$infile\n";
      $comd = "./rasp2jp2.pl $infile $jp2file";
      if (!system($comd)) {
	system("zip -m $zipfile -n jp2 $jp2file > /dev/null");
      } else {
	print "$comd\n";
	exit(1);
      }
      if ($is_zip) {
	$comd = "rm $infile";
	system($comd);
      };
    }
   }
  }
}

system("mv $zipfile xcsoar-rasp.dat");

# wstar Thermal updraft velocity
# zsfclcl Cu Cloudbase
# zwblmaxmin MSL height of maxmin Wbl

#Need further processing:
# wblmaxmin BL max up/down motion (negative)
#
# zblcl Overcast Development cloudbase
# zblcldif Overcast Development potential
# zblclmask OD cloudbase where OD potential>0
#
# zsfclcldif Cu Potential..?
# zsfclclmask  Cu Cloudbase where Cu potential>0
