package XCS::WaypointDetail;
use strict;
use warnings;
use base qw/XCS::Base/;
use XCS::Cache;
# (XXX Broken) use Image::TestJPG;

=head1 NAME

XCS::WaypointDetail - Collect waypoint details (Satellite images)

=head1 SYNOPSIS

	use XCS::C302;
	use XCS::WaypointDetail;

	# Parse a waypoint file
	my $c = XCS::C302->new();
	$c->parseFile("testpoints.dat");
	
	# Pass in a waypoint set (theoretically type independent ...)
	my $wp = XCS::WaypointDetail->new();
	$wp->waypoints($c);

=cut

sub new {
	my ($class) = @_;
	my $self = bless {}, ref($class) || $class;

	# XXX Temporary cache location !
	$self->cache(XCS::Cache->new("/usr/local/var/xcsoar/xcscache/satellite"));

	return $self;
}

sub waypoints {
	my ($self, $wp) = @_;
	$self->{WAYPOINTS} = $wp if (defined($wp));
	return $self->{WAYPOINTS};
}

sub cache {
	my ($self, $cache) = @_;
	$self->{CACHE} = $cache if (defined($cache));
	return $self->{CACHE};
}

=head2 url (x, y)

Turn x/y coordinates into the URL to get the image
XXX Or should I be doing a waypoint !

=cut

sub url {
	my ($self, $wp) = @_;
	my $x = $wp->x_norm;
	my $y = $wp->y_norm;

	# XXX Constants
	my $zoom = 0.0002778;
	my $width = 238;
	my $height = 269;
	return "http://onearth.jpl.nasa.gov/landsat.cgi?zoom=$zoom&x0=$x&y0=$y&action=zoomin&layer=modis%252Cglobal_mosaic&pwidth=$width&pheight=$height";
}

=head2 download_url (x, y)

Get the URL and download it - currently return a string

=cut

sub download_url {
        my ($self, $url) = @_;
        my $url_data = $self->get_url($url);
        foreach my $line (split(/[\r\n]/, $url_data)) {
		# XXX Make this MUCH more robust...
		# 	(eg: type="image" or out of order, spaceâ case etc)
                if ($line =~ /<input type=image src=\"(.*)\"/) {
                        my $curl = "http://onearth.jpl.nasa.gov/$1";
                        return $self->get_url($curl, $url);
                }
        }
	$self->download_delay();
	return undef;
}

# Cache filename
sub filename {
	my ($self, $wp) = @_;
	my $fn = sprintf("image_x_%03.5f_y_%03.5f.jpg", $wp->x_norm, $wp->y_norm);
	return $fn;
}


sub update_all {
  my ($self) = @_;
  
  my $tries = 5;
  my $failures = 0; # failures this iteration
  for (my $try = 1; $try <= $tries; ++$try) {
    $self->verbose("WaypointDetail - updating images, pass $try of $tries");
    $failures = 0;
    foreach my $wp ($self->waypoints->entries()) {
      $self->update($wp);
      if (!$self->check($wp)) {
	$failures++;
      }
    }
    if ($failures == 0) {
      $self->verbose("WaypointDetail - all images downloaded in pass $try");
      return 0;
    }
    for (my $i = 0; $i < 10; ++$i) {
      $self->download_delay();
    }
  }
  $self->verbose("WaypointDetail - giving up with $failures failures after $tries passes");
  return $failures;
}

=head2 check

Check that self is up to date - really just check if file exists in cache

=cut

sub check {
	my ($self, $wp) = @_;
	return $self->cache->test($self->filename($wp));
}

=head2 update

Automatically update - update from net, if it does not exist in cache

=cut

sub update {
	my ($self, $wp) = @_;
	unless ($self->check($wp)) {
		$self->download($wp);
		return 1;
	}
	return 0;
}

=head2 download

Download a single waypoint image - don't check it is there - just get it.

=cut

sub download {
  my ($self, $wp) = @_;
  $self->verbose("WayPointDetail: Updating cache - " . $self->filename($wp));
  my $image_data = $self->download_url($self->url($wp));
  #if (Image::TestJPG::testJPG($image_data, length($image_data))) {
    $self->cache->write($self->filename($wp), 
			$image_data);
  #} else {
  #  $self->verbose("WayPointDetail: bad image data for " . 
	#	   $self->filename($wp));
  #}
}

=head2 copy_all

Copy all of the waypoint files to the new filename format, directory provided

=cut

sub copy_all {
  my ($self, $dest, $normalize) = @_;
  
  foreach my $wp ($self->waypoints->entries()) {
    if ($normalize) {
      # normalize and copy in one step
      my $cmd = "convert " . $self->cache->filename($self->filename($wp)) . 
	" -normalize " . $dest . "/" . sprintf("modis-%03d.jpg",
					       $wp->index);
      print("Running \"$cmd\"\n");
      system($cmd);
    } else {
      # just copy
      $self->cache->copyfile_to($self->filename($wp),
				$dest . "/" . sprintf("modis-%03d.jpg",
						      $wp->index));
    }
  }
}

1;
