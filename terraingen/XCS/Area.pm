package XCS::Area;
use Class::MethodMaker [
			scalar => [qw/ top bottom left right /],
			new => [qw/ -hash new /],
		       ];

use Geo::Coordinates::DecimalDegrees;
use Geo::Distance;
use List::Util qw(min max);
use strict;

sub top_dms {
  my($self) = shift;
  if (@_) {
    my($degrees,$minutes,$seconds) = @_;
    $self->top(dms2decimal($degrees,$minutes,$seconds));
    return($degrees,$minutes,$seconds);
  }
  return(decimal2dms($self->top()));
}

sub format_helper {
  my($degrees, $minutes, $seconds, $directions) = @_;
  my $direction = substr($directions, $degrees < 0, 1);
  if ($degrees < 0) {
    $degrees = -$degrees;
  } 
  return sprintf("%d&deg;%d'%d''%s", $degrees, $minutes, $seconds, $direction);  
}

sub top_dms_formatted {
  my $self = shift;
  return format_helper($self->top_dms(), "NS");
}

sub bottom_dms {
  my($self) = shift;
  if (@_) {
    my($degrees,$minutes,$seconds) = @_;
    $self->bottom(dms2decimal($degrees,$minutes,$seconds));
    return($degrees,$minutes,$seconds);
  }
  return(decimal2dms($self->bottom()));
}

sub bottom_dms_formatted {
  my $self = shift;
  return format_helper($self->bottom_dms(), "NS");
}

sub left_dms {
  my($self) = shift;
  if (@_) {
    my($degrees,$minutes,$seconds) = @_;
    $self->left(dms2decimal($degrees,$minutes,$seconds));
    return($degrees,$minutes,$seconds);
  }
  return(decimal2dms($self->left()));
}

sub left_dms_formatted {
  my $self = shift;
  return format_helper($self->left_dms(), "EW");
}

sub right_dms {
  my($self) = shift;
  if (@_) {
    my($degrees,$minutes,$seconds) = @_;
    $self->right(dms2decimal($degrees,$minutes,$seconds));
    return($degrees,$minutes,$seconds);
  }
  return(decimal2dms($self->right()));
}

sub right_dms_formatted {
  my $self = shift;
  return format_helper($self->right_dms(), "EW");
}

sub validate {
  my($self) = @_;
  
  if ($self->bottom() == $self->top()) {
    return "The specified area has no height";
  }
  if ($self->left() == $self->right()) {
    return "The specified area has no width";
  }
  if ($self->bottom() > $self->top()) {
    return "The bottom of the area must be below the top of the area.";
  }
  if ( (($self->left() < $self->right()) and 
	(($self->right() - $self->left()) > 180)) or
       (($self->right() < $self->left()) and 
	(($self->right() + 360 - $self->left()) > 180)) ) {
    return "The specified area is more than 180&deg; wide, did you mix the left and right coordinates?";
  }
  
  return undef; # no error
}

sub width {
  my $self = shift;
  my($left,$right) = ($self->left(),$self->right());
  if ($left > $right) {
    $right += 360;
  }
  return $right - $left;
}

sub height {
  my $self = shift;
  return $self->top() - $self->bottom();
}

sub dimensions {
  my($self) = @_;
  my $geo = new Geo::Distance;
  
  my $width = $geo->distance('kilometer', $self->left(), $self->bottom() => 
			     $self->right(), $self->bottom());
  my $height = $geo->distance('kilometer', $self->left(), $self->bottom() =>
			      $self->left(), $self->top());
  return($width,$height);
}

sub expand {
  my($self,$kilometers) = @_;
  
  # rather than figuring out the math, I'm going to abuse
  # Geo::Distance to guess at the right numbers
  my $geo = new Geo::Distance;
  
  # find out how big one degree of latitude is in kilometers
  # yes, this is a constant...
  my $latdegree = $geo->distance('kilometer', 0, 0 => 0, 1);
  
  # find out how bit one degree of longitude is at the smaller 
  # end of the rectangle
  my $latitude_to_test = abs($self->bottom()) > abs($self->top()) ? $self->bottom() : $self->top();
  my $longdegree = $geo->distance('kilometer', 0, $latitude_to_test => 1, $latitude_to_test);
  
  my $latmargin = $kilometers / $latdegree;
  my $longmargin = $kilometers / $longdegree;
  
  $self->top(min($self->top() + $latmargin, 90));
  $self->bottom(max($self->bottom() - $latmargin, -90));
  my $newleft = $self->left() - $longmargin;
  $self->left($newleft >= -180 ? $newleft : $newleft + 360);
  my $newright = $self->right() + $longmargin;
  $self->right($newright <= 180 ? $newright : $newright - 360);
}

1;
