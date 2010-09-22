package XCS::OpenAir::Record;

use Geo::Ellipsoid;
use Geo::Coordinates::DecimalDegrees;
use Geo::Point;
use Geo::Space;

use constant NAUTICAL_MILES_TO_METERS => 1852;

sub new {
  my $proto = shift;
  my $class = ref($proto) || $proto;
  my $self = {};

  $self->{lines} = [];
  $self->{bbox} = undef;

  bless($self, $class);
  return $self;
}

sub add_line {
  my($self,$line) = @_;
  push(@{$self->{lines}}, $line);
}

# used when calculating distances
my $ellipsoid = new Geo::Ellipsoid(ellipsoid => 'WGS84',
				   units => 'degrees');

sub add_arc_with_radius {
  my($self, $center_lat, $center_lon, $radius_in_nm) = @_;


  my $radius = $radius_in_nm * NAUTICAL_MILES_TO_METERS;

  my @cardinal_points;
  for my $bearing (qw(0 90 180 270)) {
    my($lat, $lon) = $ellipsoid->at($center_lat,$center_lon,$radius,$bearing);
    push(@cardinal_points, Geo::Point->latlong($lat,$lon));
  }
  if ($self->{bbox}) {
    my($oldxmin, $oldymin, $oldxmax, $oldymax) = @{$self->{bbox}};
    my $lower_left = Geo::Point->latlong($oldymin, $oldxmin);
    my $upper_right = Geo::Point->latlong($oldymax, $oldxmax);
    push(@cardinal_points, $lower_left, $upper_right);
  }
  my $space = new Geo::Space(@cardinal_points);
  $self->{bbox} = [$space->bbox()];
}
  
sub add_point {
  my($self, $lat, $lon) = @_;

  my @points;
  push(@points, Geo::Point->latlong($lat,$lon));
  if ($self->{bbox}) {
    my($oldxmin, $oldymin, $oldxmax, $oldymax) = @{$self->{bbox}};
    my $lower_left = Geo::Point->latlong($oldymin, $oldxmin);
    my $upper_right = Geo::Point->latlong($oldymax, $oldxmax);
    push(@points, $lower_left, $upper_right);
  }
  my $space = new Geo::Space(@points);
  $self->{bbox} = [$space->bbox()];
}
  
sub bbox {
  my $self = shift;
  if ($self->{bbox}) {
    return @{$self->{bbox}};
  }
  return undef;
}

sub lines {
  my $self = shift;
  return @{$self->{lines}};
}

1;
