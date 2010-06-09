package XCS::OpenAir;
use FileHandle;
use Geo::Ellipsoid;
use Geo::Coordinates::DecimalDegrees;
use Geo::Point;
use Geo::Space;
use Tree::R;

use XCS::OpenAir::Record;

use strict;
use warnings;

use constant NAUTICAL_MILES_TO_METERS => 1852;

sub new {
  my $proto = shift;
  my $class = ref($proto) || $proto;
  my $self = {};

  $self->{'bbox'} = undef;

  bless($self, $class);
  return $self;
}

sub parse_coordinates {
  my($line) = @_;
  if ($line =~ /^\s*(\d+)(:([\d\.]+))?\s*([NS])\s+(\d+)(:([\d\.]+))?\s*([EW])/i) {
    #print "Got matches: ", join(" || ", ($1, $3, $4, $5, $7, $8)), "\n";
    my $lat_degrees = ($4 eq "N" or $4 eq "n") ? $1 : -$1;
    my $lon_degrees = ($8 eq "E" or $8 eq "e") ? $5 : -$5;
    return (dm2decimal($lat_degrees, $3), dm2decimal($lon_degrees,$7));
  } elsif ($line =~ /^\s*(\d+)(:([\d]+))?(:([\d\.]+))?\s*([NS])\s+(\d+)(:([\d]+))?(:([\d\.]+))?\s*([EW])/i) {
    #print "Got matches: ", join(" || ", ($1, $3, $5, $6, $7, $9, $11, $12)), " in $line\n";
    my $lat_degrees = ($6 eq "N" or $6 eq "n") ? $1 : -$1;
    my $lon_degrees = ($12 eq "E" or $12 eq "e") ? $7 : -$7;
    return (dms2decimal($lat_degrees, $3, $5), dms2decimal($lon_degrees,$9,$11));
  } else {
    print "$line\n";
    my $p = Geo::Point->fromString("wgs84 $line");
    print "GP got ", $p->lat(), ", ", $p->long(), "\n";
    die "unable to parse coordinates \"$line\"";
  }
}

sub read_airspace_file {
  my($self,$filename) = @_;
  my $fh = new FileHandle("<$filename")
    or die "Unable to open $filename: $@";
  my $current_record;
  
  # variable assignments
  my $D = '+';
  my $X = undef;
  my $W = undef;
  my $Z = undef;

  while (<$fh>) {
    chomp;
    next if /^\*/;  # skip comments
    next if /^\s*$/; # skip blank lines
    if (/^AC/i) {
      # we've encountered a new record, so save off the old one
      push(@{$self->{records}}, $current_record) if $current_record;
      $current_record = new XCS::OpenAir::Record;
      $current_record->add_line($_);
      $D = '+';
    } elsif ( /^A[NHL]/ or /^S[PB]/i ) {
      # these don't define 2D bounds, just store the lines
      # AN = airspace name
      # AH = ceiling
      # AL = floor
      # SP = select pen
      # SB = select brush
      $current_record->add_line($_);
    } elsif (/^(AT|DP|DY) (.*)$/i) {
      # all take a single coordinate
      # AT specifies coordinate for name
      # DP adds a polygon point
      # DY adds an airway segment
      $current_record->add_line($_);
      my($lat,$lon) = parse_coordinates($2);
      $current_record->add_point($lat,$lon);
    } elsif (/^V\s+([A-Z])\s*=\s*(.*)$/i) {
      # X is the center of circles and arcs, so we care about it
      # the rest we can ignore
      $current_record->add_line($_);
      if ($1 eq 'X' or $1 eq 'x') {
	my($lat,$lon) = parse_coordinates($2);
	$X = Geo::Point->latlong($lat,$lon);
      }
    } elsif (/^DC\s+([\d\.]+)/i) {
      # circle with radius specified
      $current_record->add_line($_);
      $current_record->add_arc_with_radius($X->lat(), $X->long(),$1);
    } elsif (/^DA\s+([\d\.]+),/i) {
      # arc with radius and two angles
      $current_record->add_line($_);
      $current_record->add_arc_with_radius($X->lat(), $X->long(),$1);
    } elsif (/^DB\s+([^,]+),(.*)$/i) {
      # arc with two coordinates
      $current_record->add_line($_);
      # calculate the radius at each coordinate, then
      # add an arc with the bigger radius
      my($first_lat,$first_lon) = parse_coordinates($1);
      my($second_lat,$second_lon) = parse_coordinates($2);
      my $ellipsoid = new Geo::Ellipsoid(ellipsoid => 'WGS84',
					 units => 'degrees');
      my $first_radius = $ellipsoid->range($first_lat, $first_lon,
					   $X->lat(), $X->long()) /
					     NAUTICAL_MILES_TO_METERS;
      my $second_radius = $ellipsoid->range($second_lat, $second_lon,
					    $X->lat(), $X->long()) / 
					      NAUTICAL_MILES_TO_METERS;
      $current_record->add_arc_with_radius($X->lat(), $X->long(),
					   ($first_radius > $second_radius) ?
					   $first_radius : $second_radius);
      
    } else {
      die "Unrecognized line: \"$_\"\n";
    }
  }
  if ($current_record) {
    push(@{$self->{records}}, $current_record);
    $current_record = undef;
  }
  
}

sub generate_airspace_for_region {
  my($self, $output_filename, $xmin, $ymin, $xmax, $ymax) = @_;
  my $rtree = new Tree::R;
  for my $record (@{$self->{records}}) {
    my @bbox = $record->bbox();
    $rtree->insert($record, @bbox);
  }
  my @results = ();
  my @rect = ($xmin, $ymin, $xmax, $ymax);
  $rtree->query_partly_within_rect(@rect, \@results);
  if (@results) {
    my $fh = new FileHandle(">$output_filename");
    for my $record (@results) {
     $fh-> print(join("\n", $record->lines()), "\n\n");
   }
  }
}

sub test {
  my @rect = (-82, 36.5, -78.5, 38.5);
    
}

package main;

#my $rec = new XCS::OpenAir::Record;
#$rec->add_arc_with_radius(38.5, 82, 10);
#my($xmin,$ymin,$xmax,$ymax) = $rec->bbox();
#print "Space bounded by $xmin,$ymin,$xmax,$ymax\n";

#my $airspace = new XCS::OpenAir;
#$airspace->read_airspace_file("airspace.dat");
#$airspace->read_airspace_file("CONUS.TXT");
#$airspace->test();
1;
