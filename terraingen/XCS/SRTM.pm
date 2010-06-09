package XCS::SRTM;
use POSIX qw(ceil floor);
use File::Temp;

use XCS::Config;
use warnings;

use strict;

sub new {
  my ($class, @rest) = @_;
  my $self = bless {}, ref($class) || $class;
  #$self->init(@rest);
  return $self;
}

sub make_srtm_filename {
  my($latitude, $longitude) = @_;
  return sprintf("%s%02d%s%03d.tif", 
		 ($latitude < 0 ? 'S' : 'N'),
		 abs($latitude),
		 ($longitude < 0 ? 'W' : 'E'),
		 abs($longitude));
}

sub prepare_srtm_tile {
  my($base_filename,$tempdir) = @_;
  my $destination_filename = "$tempdir/$base_filename";
  my $tile_directory = $XCS::Config::srtm3v2_directory;
  if (-f "$tile_directory/${base_filename}.bz2") {
    print "Uncompressing tile ${base_filename}.bz2\n";
    system("bzip2 -dc $tile_directory/${base_filename}.bz2 > $destination_filename");
    return $destination_filename;
  }
  if (-f "$tile_directory/${base_filename}.gz") {
    print "Uncompressing tile ${base_filename}.gz\n";
    system("gzip -dc $tile_directory/${base_filename}.gz > $destination_filename");
    return $destination_filename;
  }
  if (-f "$tile_directory/$base_filename") {
    if (XCS::Config::copy_uncompressed_srtm_tiles()) {
      print "Copying tile $base_filename\n";
      copy("$tile_directory/$base_filename", $destination_filename);
      return $destination_filename;
    } else {
      print "Using tile $base_filename in place\n";
      return "$tile_directory/$base_filename";
    }
  }
  print("Unable to find tile $tile_directory/$base_filename\n");
  return undef;
}

sub set_area {
  my($self, $area) = @_;
  $self->{'area'} = $area;
}

sub set_area_name {
  my($self,$area_name) = @_;
  $self->{'area_name'} = $area_name;
}

sub set_resolution {
  my($self,$resolution) = @_;
  $self->{'resolution'} = $resolution;
}

sub collect_tiles {
  my ($self) = @_;
  my $area = $self->{'area'};
  my ($top, $bottom, $left, $right) =
    ($area->top(), $area->bottom(), $area->left(), $area->right());

  my @filename_list;
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);

  my $whole_degree_latitude = floor($bottom);
  my $whole_degree_longitude = floor($left);
  
  for (my $latitude_cursor = floor($bottom);
       $latitude_cursor < ceil($top);
       $latitude_cursor++) {
    my $longitude_cursor_min = floor($left);
    my $longitude_cursor_max = ceil($right);
    if ($longitude_cursor_min > $longitude_cursor_max) {
      $longitude_cursor_max += 360;
    }
    for (my $longitude_cursor_raw = $longitude_cursor_min ;
	 $longitude_cursor_raw < $longitude_cursor_max;
	 $longitude_cursor_raw++) {
      print("$latitude_cursor, $longitude_cursor_raw\n");
      my $longitude_cursor = $longitude_cursor_raw > 179 ? 
	$longitude_cursor_raw - 360 : 
	  $longitude_cursor_raw;
      my $base_filename = make_srtm_filename($latitude_cursor,$longitude_cursor);
      my $filename = prepare_srtm_tile($base_filename, $tempdir);
      if ($filename) {
	push(@filename_list, $filename);
      }
    }
  }
  return @filename_list;
}

sub merge {
  my($self,@filenames) = @_;
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);
  my $out_filename = File::Spec->catfile($tempdir, $self->{'area_name'} . "_merged.tif");
  my $command = "gdal_merge.py -n -32768 -init -32768 -o $out_filename " . join(" ", @filenames);
  print "Running $command\n";
  system($command);
  return $out_filename;
}

sub resample {
  my($self, $source_file) = @_;
  my $degrees_per_pixel = 1 / 60 / 60 * $self->{'resolution'};
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);
  my $out_filename = File::Spec->catfile($tempdir, $self->{'area_name'} . "_resampled.tif");
  my $command = "gdalwarp -rcs -tr $degrees_per_pixel $degrees_per_pixel -wo \"INTERLEAVE=BIL\" -wt Int16 -of \"GTiff\" -srcnodata -32768 -dstnodata -1 $source_file $out_filename";
  print "Running $command\n";
  system($command);
  system("cp $out_filename /tmp");
  return $out_filename;
}  

sub crop {
  my($self, $source_file) = @_;
  my $area = $self->{'area'};
  my($top,$bottom,$left,$right) = ($area->top(), $area->bottom(), $area->left, $area->right());
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);
  my $out_filename = File::Spec->catfile($tempdir, $self->{'area_name'} . "_cropped.tif");
  my $command = "gdalwarp -srcnodata -1 -dstnodata -1 -te $left $bottom $right $top $source_file $out_filename";
  print("Running $command\n");
  system($command);
  system("cp $out_filename /tmp");
  return $out_filename;
}  

sub convert_to_xcsoar {
  my($self, $filename, $dest_dir) = @_;
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);
  my $erm_filename = File::Spec->catfile($tempdir, $self->{'area_name'} . "_cropped.tif");
  my $info_filename = File::Spec->catfile($tempdir, $self->{'area_name'} . "_cropped.info");
  my $command = "gdalinfo $filename > $info_filename";
  print "Running $command\n";
  system($command);
  $command = "gdalwarp -ot Int16 -of \"ENVI\" $filename $erm_filename";
  print "Running $command\n";
  system($command);

  my $infofh = new IO::File($info_filename);
  my($stepsize,$lat,$lon,$columns,$rows);
  while(<$infofh>) {
    print("Reading info: $_\n");
    if (/Pixel Size\s+=\s+\((\S+),\S+\)/) {
      $stepsize = $1;
    }
    if (/Origin = \((\S*),(\S*)\)/) {
      $lat = $2;
      $lon = $1;
    }
    if (/Size is (\S+),\s(\S+)/) {
      $columns = $1;
      $rows = $2;
    }
  }
  $infofh->close();

  ########################
  
  my $top = $lat;
  my $bottom = $lat-$stepsize*$rows;
  my $left = $lon;
  my $right = $lon+$stepsize*$columns;

  ########
  my $output_filename = File::Spec->catfile($dest_dir, $self->{'area_name'} . "-dem.dat");

  my $demheader = pack("dddddll", $left, $right, $top, $bottom, $stepsize,
		       $rows, $columns);
  
  print "$columns $rows\n";
  my $outfile = new IO::File(">$output_filename");
  binmode $outfile;
  $outfile->print($demheader);
  my $infile = new IO::File("<$erm_filename");
  binmode $infile;
  for (my $i=0; $i<$rows; $i++) {
    my $ermdata;
    $infile->read($ermdata,2*$columns);
    $outfile->print($ermdata);
  }
  $infile->close();
  $outfile->close();
  return $output_filename;
}


sub geo_compress {
  my($self,$sourcefile,$dest_dir) = @_;
  my $out_filename = File::Spec->catfile($dest_dir, "terrain.jp2");
  my $comd = "geojasper -f $sourcefile -F $out_filename -T jp2 -O rate=0.1 -O tilewidth=256 -O tileheight=256 -O xcsoar=1";
  system($comd);
  system("cp $out_filename /tmp");
  return $out_filename;
}

sub generate {
  my($self,$dest_dir,$generate_xcm) = @_;
  my @filenames = $self->collect_tiles();
  if (!@filenames) {
    return undef;
  }
  my $merged_file = $self->merge(@filenames);
  my $resampled_file = $self->resample($merged_file);
  my $cropped_file = $self->crop($resampled_file);
  if ($generate_xcm) {
  	return $self->geo_compress($cropped_file, $dest_dir);
  } else {
	  my $final_file = $self->convert_to_xcsoar($cropped_file, $dest_dir);
	  return $final_file;
  }
}

1;
