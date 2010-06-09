package XCS::TerrainGenerator;
use strict;
use CGI::Carp qw(fatalsToBrowser);
use base 'CGI::Application';
use CGI::Application::Plugin::Session;
use CGI::Application::Plugin::Stream qw(stream_file);
use HTML::FillInForm;
use XCS::Config;
use XCS::Area;
use File::CacheDir;
use File::Copy;
use File::Temp;
use File::Basename;
use Number::Bytes::Human;
use XCS::C302;
use XCS::WaypointDetail;
use Config::Simple;
use XML::DOM;
use LWP::UserAgent;
use HTML::TreeBuilder;

my $KILOMETERS_IN_A_MILE = 1.609344;

our $tx_sites;
require '/usr/local/share/xcsoar/tx_sites.pl';

sub setup {
  my $self = shift;
  $self->tmpl_path($XCS::Config::html_template_path);
  $self->run_modes(
		   'initial' => 'show_initial_form',
		   'process_initial_form' => 'process_initial_form',
		   'coords_from_tx' => 'coordinates_from_tx',
		   'process_coords_from_tx' => 'process_coordinates_from_tx',
		   'coords_from_file' => 'coordinates_from_file',
		   'process_coords_from_file' => 'process_coordinates_from_file',
		   'confirm_from_file' => 'confirm_coordinates_from_file',
		   'process_confirm_from_file' => 'process_confirm_coordinates_from_file',
		   'coords_from_user' => 'coordinates_from_user',
		   'process_coords_from_user' => 'process_coordinates_from_user',
		   'srtm_resolution_form' => 'srtm_resolution_form',
		   'process_srtm_resolution_form' => 'process_srtm_resolution_form',
		   'request_completed' => 'terrain_request_completed',
		   'status' => 'terrain_show_status',
		   'download' => 'terrain_download',
		   'download_file' => 'download_file',
		   );
  $self->start_mode('initial');
  $self->mode_param('rm');
}

sub cgiapp_init {
  my $self = shift;
  # use Base32 for shorter session IDs, since we will be putting them
  # in an email
  # // JMW was Base32 now md5
  $self->session_config(CGI_SESSION_OPTIONS => [ "driver:File;id:md5;serializer:Storable", $self->query(), { Directory => $XCS::Config::session_directory } ], DEFAULT_EXPIRY => "+1w", SEND_COOKIE => 1);
}

sub cgiapp_postrun {
  my $self = shift;
  
  # the 'degree' symbol doesn't display correctly unless the browser
  # is using utf-8 encoding
  $self->header_add(-type => 'text/html; charset=utf-8');
}

sub show_initial_form {
  my $self = shift;
  my $errs = shift;

  my $template = $self->load_tmpl("initial.tmpl");

  $template->param($errs) if $errs;
  return $template->output();
}

sub process_initial_form {
  my $self = shift;

  use CGI::Application::Plugin::ValidateRM (qw/check_rm/);
  my($results, $err_page) = $self->check_rm('show_initial_form',
					    'get_initial_form_constraints');
  return $err_page if $err_page;

  # no errors, save off the data
  my $q = $self->query();
  my $s = $self->session();
  $s->param('area_name', $q->param('areaname'));
  $s->param('generate_xcm', $q->param('generate_xcm'));
  $s->param('user_email', $q->param('email'));
  $s->param('distance_units', $q->param('distance_units') eq 'sm' ? 'sm' : 'km');
  $s->param('selection_method', $q->param('selection_method'));
  if ($q->param('selection_method') eq 'manual') {
    return $self->coordinates_from_user();
  } elsif ($q->param('selection_method') eq 'tx') {
    return $self->coordinates_from_tx();
  } else {
    return $self->coordinates_from_file();
  }
}

sub get_initial_form_constraints {
  return 
    {
     required => [qw(areaname email email_confirm)],
     filters => ['trim'],
     constraints =>
     {
      areaname => qr/^\w{1,20}$/,
      email => [
		{
		 constraint => 'email',
		},
		{
		 name => 'email_mismatch',
		 constraint => sub {
		   my($email1,$email2) = @_;
		   return $email1 eq $email2;
		 },
		 params => [qw(email email_confirm)],
		},
	       ],
     },
     msgs => {
	      constraints => {
			      'email_mismatch' => 'email addresses differ',
			     },
	      prefix => "err_",
	     },
    }; 
}

##
# retrieve the cambridge .dat file for the given soaring site from
# the tx mirror specified
# returns the contents of the waypoint file, or undef if error
sub get_waypoint_from_tx_site {
  my($mirror,$site) = @_;
  my $url = "$mirror/$site/files.html";
  my $ua = new LWP::UserAgent();
  $ua->timeout(10);
  my $response = $ua->get($url);
  if ($response->is_success()) {
    my $tree = new HTML::TreeBuilder();
    $tree->parse($response->content());
    $tree->eof();
    # look for the first linked file that ends in .dat and contains no
    # slashes (some pages have links to original source data on other servers)
    my @a_links = $tree->look_down('_tag', 'a', sub { $_[0]->attr('href') and $_[0]->attr('href') =~ /^[^\/]+\.dat$/i });
    if (@a_links) {
      my $dat_url = "$mirror/$site/" . $a_links[0]->attr('href');
      $response = $ua->get($dat_url);
      return $response->is_success() ? $response->content() : undef;
    }
  }
  return undef;
}

# try to download the waypoint file from the turnpoint exchange 
# using all mirrors.  returns the contents of the waypoint file or undef
# on error
sub get_waypoint_from_tx {
  my($site) = @_;
  my @mirrors = qw(http://soaring.aerobatics.ws/TP
		 http://soaring.xinqu.net/TP
		 http://soaring.gahsys.com/TP);
  for my $mirror (@mirrors) {
    if (my $contents = get_waypoint_from_tx_site($mirror,$site)) {
      return $contents;
    }
  }
  return undef;
}

sub coordinates_from_tx {
  my $self = shift;
  my $errs = shift;

  # re-sort the tx site list by country
  my %sites_by_country;
  for my $site (sort keys %{$tx_sites}) {
    if (!defined($sites_by_country{$tx_sites->{$site}->{country}})) {
      $sites_by_country{$tx_sites->{$site}->{country}} = [];
    }
    $tx_sites->{$site}->{short_name} = $site;
    push(@{$sites_by_country{$tx_sites->{$site}->{country}}},
	 $tx_sites->{$site});
    
  }

  my @data_by_country;
  for my $country (sort(keys(%sites_by_country))) {
    my @site_data;
    
    for my $site (@{$sites_by_country{$country}}) {
      my $js = "document.forms[0].site.value = '" . $site->{short_name} . "'; document.forms[0].submit();";
      my $tag = "<a href=\"javascript:$js\">" . 
	$site->{name} . "</a>";
      push(@site_data, { site_tag => $tag, site_name => $site->{name}, short_name => $site->{short_name} } );
    }
    my @sorted_site_data = sort { $a->{site_name} <=> $b->{site_name} } @site_data;
    my $country_link = "<a href=\"#$country\">$country</a>";
    my $country_target = "<a name=\"$country\">$country</a>";
    push(@data_by_country, { country_name => $country, 
			     country_link => $country_link,
			     country_target => $country_target,
			     sites => \@sorted_site_data } );
  }
      
  
  # I think die_on_bad_params needs to be off because of buggy handling of
  # nested loops in HTML::Template
  my $template = $self->load_tmpl("tx_flat_list.tmpl", die_on_bad_params => 0);

  $template->param('data_by_country', \@data_by_country);
  $template->param($errs) if $errs;
  return $template->output();
}

sub process_coordinates_from_tx {
  my $self = shift;
  my $q = $self->query();
  my $site_short = $q->param('site');
  if ($site_short) {
    my $contents = get_waypoint_from_tx($site_short);
    if ($contents) {
      my $cache_dir = new File::CacheDir(base_dir => $XCS::Config::temp_directory, ttl => '1 day', filename => 'waypoint-' . time . ".$$");
      my $saved_waypoint_filename = $cache_dir->cache_dir();
      my $waypoint_fh = new FileHandle(">$saved_waypoint_filename");
      $waypoint_fh->print($contents);
      $waypoint_fh->close();

      my $waypoints = new XCS::C302;
      # bad error handling for now
      eval { $waypoints->parseAny($saved_waypoint_filename); };
      if ($@) {
	return $self->coordinates_from_file({waypoint_file_error => $@});
      }
      
      my $s = $self->session();
      $s->param('waypoint_file', $saved_waypoint_filename);
      return $self->confirm_coordinates_from_file();
    }
  } else {
    return $self->coordinates_from_file({waypoint_file_error => "Please upload a waypoint file"});
  }
}

sub coordinates_from_file {
  my $self = shift;
  my $errs = shift;

  my $template = $self->load_tmpl("from_file.tmpl");

  $template->param($errs) if $errs;
  return $template->output();
}

sub process_coordinates_from_file {
  my $self = shift;
  my $q = $self->query();
  if ($q->param('waypoint_file')) {
    my $cache_dir = new File::CacheDir(base_dir => $XCS::Config::temp_directory, ttl => '1 day', filename => 'waypoint-' . time . ".$$");
    my $saved_waypoint_filename = $cache_dir->cache_dir();
    File::Copy::copy($q->upload('waypoint_file'), $saved_waypoint_filename);
    
    my $waypoints = new XCS::C302;
    # bad error handling for now
    eval { $waypoints->parseAny($saved_waypoint_filename); };
    if ($@) {
      return $self->coordinates_from_file({waypoint_file_error => $@});
    }
    
    my $s = $self->session();
    $s->param('waypoint_file', $saved_waypoint_filename);
    return $self->confirm_coordinates_from_file();
  } else {
    return $self->coordinates_from_file({waypoint_file_error => "Please upload a waypoint file"});
  }
}

sub confirm_coordinates_from_file {
 
  my $self = shift;  
  my $errs = shift;

  my $template = $self->load_tmpl("confirm_from_file.tmpl");

  my $s = $self->session();
  my $waypoints = new XCS::C302;
  $waypoints->parseAny($s->param('waypoint_file'));

  my $area = new XCS::Area;
  $area->top($waypoints->y_max());
  $area->bottom($waypoints->y_min());
  $area->left($waypoints->x_min());
  $area->right($waypoints->x_max());

  $template->param('top_formatted', $area->top_dms_formatted());
  $template->param('bottom_formatted', $area->bottom_dms_formatted());
  $template->param('left_formatted', $area->left_dms_formatted());
  $template->param('right_formatted', $area->right_dms_formatted());

  $s->param('area', $area);

  $template->param("distance_units", $s->param("distance_units"));

  my @waypoint_entries = $waypoints->entries();
  my @waypoint_data = map { { 'title' => $_->title(), 'x' => $_->x(), 'y' => $_->y() } } @waypoint_entries;
  $template->param('turnpoint_list', \@waypoint_data);
  $template->param($errs) if $errs;
  return $template->output();
}

sub process_confirm_coordinates_from_file {
  my $self = shift;
  use CGI::Application::Plugin::ValidateRM (qw/check_rm/);
  my($results, $err_page) = $self->check_rm('confirm_coordinates_from_file',
					    'confirm_coordinates_from_file_constraints');
  return $err_page if $err_page;

  my $q = $self->query();
  my $s = $self->session();
  my $area = $s->param('area');
  my $km_to_inflate = $q->param('border');
  if ($s->param('distance_units') eq 'sm') {
    $km_to_inflate = $km_to_inflate * $KILOMETERS_IN_A_MILE;
  }
  $area->expand($km_to_inflate);
  $s->param('area',$area);
  if ($q->param('generate_waypoint_images')) {
    $s->param('generate_waypoint_images', 'true');
  }
  if ($q->param('normalize_waypoint_images')) {
    $s->param('normalize_waypoint_images', 'true');
  }
  return $self->srtm_resolution_form();
}

sub confirm_coordinates_from_file_constraints {
#add more constraints like those in coordinates_from_user_constraints
  return 
    {
     required => [qw(border)],
     filters => ['trim'],
     constraints =>
     {
      border => qr/^\d+$/,
     },
     msgs => {
	      prefix => "err_",
	     },
    }; 
}

sub coordinates_from_user {
  my $self = shift;
  my $errs = shift;

  my $template = $self->load_tmpl("coordinate_entry.tmpl");

  $template->param($errs) if $errs;
  return $template->output();
}

# on the form, a coordinate is separated into four inputs -- degrees,
# minutes, seconds, and cardinal direction.  This reads the four inputs
# and combines them into one (signed, decimal) number.
sub read_coordinate_from_form {
  my($q, $prefix) = @_;
  my $direction = $q->param("${prefix}_direction");
  my $direction_modifier = (($direction eq 'S') or
			    ($direction eq 'W')) ? -1 : 1;
  return($q->param("${prefix}_degrees") * $direction_modifier,
	 $q->param("${prefix}_minutes"),
	 $q->param("${prefix}_seconds"));
}

sub process_coordinates_from_user {
  my $self = shift;
  use CGI::Application::Plugin::ValidateRM (qw/check_rm/);
  my($results, $err_page) = $self->check_rm('coordinates_from_user',
					    'coordinates_from_user_constraints');
  return $err_page if $err_page;

  # the page has passed all of the simple validation, meaning each of 
  # the four coordinates makes sense on its own.  For this page, 
  # we will do a second level of validation to ensure the four coordinates
  # describe a sensible rectangle

  my $q = $self->query();

  my $area = new XCS::Area;
  $area->top_dms(read_coordinate_from_form($q, "top"));
  $area->bottom_dms(read_coordinate_from_form($q, "bottom"));
  $area->left_dms(read_coordinate_from_form($q, "left"));
  $area->right_dms(read_coordinate_from_form($q, "right"));
  
  my $error_message = $area->validate();
  if (!$error_message) {
    # make sure the user isn't generating a huge terrain file that would
    # kill the server.  I'm making these numbers up, but they are 50% larger
    # than the largest "stock" terrain currently used.
    my($width,$height) = $area->dimensions();
    if (($width * $height) >= (1500*1500)) {
      # the maximum size of terrain we will generate is 
      # a square area 1500 kilometers on a side.  This
      # corresponds to about 15 degrees at the equator.
      $error_message = "you entered an area too large for this script to handle.  Please double-check the coordinates.  If you really need the terrain for a large area, please contact efrias\@pobox.com with the coordinates and I'll try to generate it by hand.";
    }
  }
  if ($error_message) {
    # here we manually do what the check_rm function did for us -- 
    # set a variable to display an error at the top of the form, 
    # then merge the inputs back into the generated form and 
    # return it for display
    my $errs = { 'err_main' => '<span style="color:red;font-weight:bold"><span class="dfv_errors">Error: ' . $error_message . '</span></span>' };
    my $output = $self->coordinates_from_user($errs);
    my $fif = new HTML::FillInForm;
    return $fif->fill(scalarref => \$output, fobject => $q);
  }

  my $s = $self->session();
  $s->param('area', $area);
  return $self->srtm_resolution_form();
}

sub coordinates_from_user_constraints {
  my $result = 
    {
     constraints => {},
     required => [qw(left_degrees right_degrees top_degrees bottom_degrees)],
     optional => [qw(left_minutes right_minutes top_minutes bottom_minutes left_seconds right_seconds top_seconds bottom_seconds)],
     filters => ['trim'],
     msgs => 
       {
	constraints => {'out_of_range' => "Value out of range" },
	prefix => "err_",
       },
     };

  my $simple_longitude_degree_constraint =       
    [{constraint => qr/^\d*$/},
     {name => 'out_of_range',
      constraint => sub { my($d) = @_; return ($d >= 0) && ($d <= 180); }
     }];

  my $simple_latitude_degree_constraint =       
    [{constraint => qr/^\d*$/},
     {name => 'out_of_range',
      constraint => sub { my($d) = @_; return ($d >= 0) && ($d <= 90); }
     }];
  my $simple_minute_second_constraint =
    [{constraint => qr/^\d*$/},
     {name => 'out_of_range',
      constraint => sub { my($d) = @_; return ($d >= 0) && ($d < 60); }
     }];


  for my $direction (qw(top bottom)) {
    $result->{constraints}->{"${direction}_degrees"} =
      $simple_latitude_degree_constraint;
  }
  for my $direction (qw(left right)) {
    $result->{constraints}->{"${direction}_degrees"} = $simple_longitude_degree_constraint;
  }
  for my $direction (qw(top bottom left right)) {
    $result->{constraints}->{"${direction}_minutes"} =
      $simple_minute_second_constraint;
    $result->{constraints}->{"${direction}_seconds"} =
      $simple_minute_second_constraint;
  }
  return $result;
}



################
# srtm resolution selection

sub srtm_resolution_form {
  my $self = shift;
  my $errs = shift;

  my $template = $self->load_tmpl("srtm_resolution.tmpl");

  my $s = $self->session();

  my $area = $s->param('area');
  my $generate_xcm = $s->param('generate_xcm') eq 'true';
  # format the coordinates for display
  $template->param('top_formatted', $area->top_dms_formatted());
  $template->param('bottom_formatted', $area->bottom_dms_formatted());
  $template->param('left_formatted', $area->left_dms_formatted());
  $template->param('right_formatted', $area->right_dms_formatted());

  # format the area's dimensions for display
  my($width,$height) = $area->dimensions();
  if ($s->param('distance_units') eq 'km') {
    $template->param('width_formatted', sprintf("%.1f km", $width));
    $template->param('height_formatted', sprintf("%.1f km", $height));
  } else {
    $template->param('width_formatted', sprintf("%.1f miles", $width / $KILOMETERS_IN_A_MILE));
    $template->param('height_formatted', sprintf("%.1f miles", $height / $KILOMETERS_IN_A_MILE));
  }

  my($width_in_degrees, $height_in_degrees) = ($area->width(), $area->height);
  for my $resolution (3,6,9,10,15,20,30,45) {
    my $width_in_pixels = $width_in_degrees * 60 * 60 / $resolution;
    my $height_in_pixels = $height_in_degrees * 60 * 60 / $resolution;
    my $filesize_bytes = $width_in_pixels * $height_in_pixels * 2;
    if ($generate_xcm) {
	$filesize_bytes *= 0.1;
    }
    my $filesize_human = Number::Bytes::Human::format_bytes($filesize_bytes);
    $template->param($resolution . "_as_size", $filesize_human);
  }

  # figure out the image size corresponding to the size of the area they
  # selected
  my $aspect_ratio = $width_in_degrees / $height_in_degrees;
  my($image_width, $image_height);
  my $image_largest_dimension = 400;
  if ($aspect_ratio < 1.0) {
    # image is tall
    $image_height = $image_largest_dimension;
    $image_width = $image_largest_dimension * $aspect_ratio;
  } else {
    # image is wide
    $image_width = $image_largest_dimension;
    $image_height = $image_largest_dimension / $aspect_ratio;
  }
  my $map_query = "/cgi-bin/mapserv?mode=map&map=/var/www/www.xcsoar.com/data/xcsoar.map&mapext=" . $area->left() . "+" . $area->bottom() . "+" . $area->right() . "+" . $area->top() . "&mapsize=" . int($image_width) . "+" . int($image_height) . "&layers=srtm30%20watercourses%20waterareas%20rail%20roads%20builtup&undefined";
  $template->param('map_img_tag', "<img src=\"$map_query\" width=\"$image_width\" height=\"$image_height\" alt=\"Terrain Preview\">");

  $template->param($errs) if $errs;
  return $template->output();
}

sub process_srtm_resolution_form {
  my $self = shift;
  my $q = $self->query();
  my $resolution = $q->param('resolution');
  my $s = $self->session();
  $s->param('srtm_resolution', $resolution);
  
  return $self->terrain_request_completed();
}

sub terrain_request_completed {
#queue-code maybe?
  my $self = shift;
  my $errs = shift;
  
  my $q = $self->query();
  my $s = $self->session() or die "Unable to load current session";
  my $sid = $s->id();
  my $url = $q->url() . "?CGISESSID=$sid&rm=download";
  
  my $config = new Config::Simple(syntax => 'simple');
  $config->param('status', 'queued');
  $config->param('generate_xcm', $s->param('generate_xcm'));
  $config->param('area_name', $s->param('area_name'));
  $config->param('email', $s->param('user_email'));
  $config->param('url', $url);
  my $area = $s->param('area') or die "Area was nil";

  $config->param('top', $area->{'top'});
  $config->param('bottom', $area->bottom());
  $config->param('left', $area->left());
  $config->param('right', $area->right());
  $config->param('resolution', $s->param('srtm_resolution'));
  if ($s->param('waypoint_file')) {
    $config->param('waypoint_file', $s->param('waypoint_file'));
#    if ($s->param('generate_waypoint_images') eq "true") {
#      $config->param('generate_waypoint_images', "true");
#    }
#    if ($s->param('normalize_waypoint_images') eq "true") {
#      $config->param('normalize_waypoint_images', "true");
#    }
  }
  
  my $job_fh = new File::Temp( TEMPLATE => "jobXXXXXX", 
			       DIR => $XCS::Config::incoming_queue_dir,
			       SUFFIX => '.txt',
			       UNLINK => 0 );
  $config->write($job_fh->filename());
  $s->param('job_filename', basename($job_fh->filename()));

  return $self->terrain_show_status();
}

sub find_job {
  my($job_filename) = shift;
  for my $dir ($XCS::Config::incoming_queue_dir,
	       $XCS::Config::working_queue_dir,
	       $XCS::Config::outgoing_queue_dir) {
    my $full_filename = File::Spec->catfile($dir,$job_filename);
    if (-f $full_filename) {
      return $full_filename;
    }
  }
  return undef;
}


sub terrain_show_status {
  my $self = shift;
  my $errs = shift;

  my $q = $self->query();

  my $s = $self->session();
  my $job_path = find_job($s->param('job_filename'));
  if (defined($job_path)) {
    my $job = new Config::Simple($job_path);
    if ($job->param('status') eq 'done') {
      undef $job; # close the config file, terrain_download will reopen
      return $self->terrain_download();
    } else {
      my $template = $self->load_tmpl("request_completed.tmpl");
      $template->param('status',$job->param('status'));
      $template->param('email',$job->param('email'));
      $template->param('source_email', $XCS::Config::from_email_address);
      $self->header_add(-Refresh => "60; URL=" . $q->url() . "?rm=status");
      return $template->output();
    }
  }
}

sub terrain_download {
  my $self = shift;
  my $errs = shift;

  my $q = $self->query();

  my $template = $self->load_tmpl("download.tmpl");

  my $s = $self->session();
  my $job_path = File::Spec->catfile($XCS::Config::outgoing_queue_dir,
				     $s->param('job_filename'));
  if ( -f $job_path) {
    my $job = new Config::Simple($job_path);
    # format the coordinates for display
    $template->param('area_name', $s->param('area_name'));
    $template->param('filename', $job->param('zipfilename'));
    $template->param('file_size', -s $job->param('zipfile'));
    #$template->param('email', $s->param('user_email'));
    
    $template->param('anchor_tag_begin', '<a href="' . $q->url(-relative => 1) . '?rm=download_file">');
    $template->param('anchor_tag_end', '</a>');
    
    $template->param($errs) if $errs;
    return $template->output();
  }
}

# actually streams the generated file to the end-user
sub download_file {
  my($self, $errs) = @_;

  my $s = $self->session();

  my $job_path = File::Spec->catfile($XCS::Config::outgoing_queue_dir,
				     $s->param('job_filename'));
  if ( -f $job_path) {
    my $job = new Config::Simple($job_path);

    local $| = 1;
    
    $self->header_add( -type => 'application/octet-stream',
		       -attachment => $job->param('zipfilename'));
    if ($self->stream_file($job->param('zipfile'))) {
      return;
    } else {
      # unable to stream file
    }
  } else {
    # job isn't ready or has expired
  }
}

1;
