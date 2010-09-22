package XCS::VMap;
use IO::File;
use File::Temp;
use File::Copy;

use XCS::Config;

use strict;
use warnings;
use base qw/XCS::Base/;

=head1 NAME

XCS::VMap - VMap stuff ???

=cut

sub new {
  my ($class, @rest) = @_;
  my $self = bless {}, ref($class) || $class;
  $self->init(@rest);
  return $self;
}

=head2 guess(x1,y1,x2,y2)

Feed in a square, guess which map.
Helps get us started...

NOTE: Class method...

=cut

sub guess {
	my ($x1,$y1,$x2,$y2) = @_;

}

sub set_area {
  my($self, $area) = @_;
  $self->{'area'} = $area;
}

sub set_area_name {
  my($self,$area_name) = @_;
  $self->{'area_name'} = $area_name;
}

sub collect_data {
  my($self)= @_;
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);
  
  my @regions = qw(v0eur/vmaplv0/eurnasia v0sas/vmaplv0/sasaus v0noa/vmaplv0/noamer v0soa/vmaplv0/soamafr);
  
  my $area = $self->{'area'};
  my $lonmin = $area->left();
  my $lonmax = $area->right();
  my $latmin = $area->bottom();
  my $latmax = $area->top();
  my $volume = $XCS::Config::topology_directory;

  system("ogr2ogr -spat $lonmin $latmin $lonmax $latmax $tempdir gltp:/vrf/$volume/" . $regions[0]);
  for (my $i = 1; $i < scalar(@regions); ++$i) {
    system("ogr2ogr -update -append -spat $lonmin $latmin $lonmax $latmax $tempdir gltp:/vrf/$volume/" . $regions[$i]);
  }
  return $tempdir;
}

sub convert_data {
  my($self, $collected_dir, $output_dir) = @_;
  my $tempdir = File::Temp::tempdir("xcsoar_XXXX", CLEANUP => 1, DIR => $XCS::Config::temp_directory);
  
  my $area = $self->{'area'};
  my $lonmin = $area->left();
  my $lonmax = $area->right();
  my $latmin = $area->bottom();
  my $latmax = $area->top();

  my $shapes = [ { inshp => "mispopp\@pop\(\*\)_point.shp", 
		   where => "", select => "txt" },
		 { inshp => "builtupa\@pop\(\*\)_area.shp", 
		   where => "", select => "nam" },
		 { inshp => "watrcrsl\@hydro\(\*\)_line.shp", 
		   where => "hyc=8",select => "nam"}, # was hyc=6
		 { inshp => "inwatera\@hydro\(\*\)_area.shp", 
		   where => "hyc=8",select => "nam"},
		 { inshp => "railrdl\@trans\(\*\)_line.shp", 
		   where => "exs=28",select => "fco"},
		 { inshp => "roadl\@trans\(\*\)_line.shp", 
		   where => "rtt=14", select => "med"} ];
  
  for my $shape (@$shapes) {
    my $sname = $shape->{inshp};
    $sname =~ s/(\S+)\@.*/$1/g;
    my $whereclause = $shape->{where} ? " -where \"" . $shape->{where} . "\"" : "";
    my $comd = "ogr2ogr $tempdir \"$collected_dir/" . $shape->{inshp} . "\"  $whereclause -select \"" . $shape->{select} . "\" -spat $lonmin $latmin $lonmax $latmax";
    system($comd);

    my $file = $shape->{inshp};
    $file =~ s/[\@\(\)\*]//g;
    $file =~ s/(\S+)\.shp/$1/g;
    my $sname2 = $shape->{inshp};
    $sname2 =~ s/(.*)\.shp/$1/g;
    copy("$tempdir/$sname2.shp", "$output_dir/$file.shp");
    copy("$tempdir/$sname2.dbf", "$output_dir/$file.dbf");
    copy("$tempdir/$sname2.shx", "$output_dir/$file.shx");
    copy("$tempdir/$sname2.prj", "$output_dir/$file.prj");
  }
}


sub generate {
  my($self, $output_directory) = @_;
  return if (! $self->{'area'} or ! $self->{'area_name'});
  my $collected_data = $self->collect_data();
  $self->convert_data($collected_data, $output_directory);
  my $area_name = $self->{'area_name'};
  my $metadata_file = new IO::File(">$output_directory/$area_name.tpl") or die "could not open >$output_directory/$area_name.tpl";
  $metadata_file->print("* filename,range,icon,field\n");
  $metadata_file->print("inwaterahydro_area, 100,,,64,96,240\n");
  $metadata_file->print("watrcrslhydro_line,    7,,,64,96,240\n");
  $metadata_file->print("builtupapop_area,  15,,1,223,223,0\n");
  $metadata_file->print("roadltrans_line,     15,,,240,64,64\n");
  $metadata_file->print("railrdltrans_line,   10,,,64,64,64\n");
  $metadata_file->print("mispopppop_point,    5,218,1,223,223,0\n");
  $metadata_file->close();
}

1;
