package XCS::Config;
use strict;
use File::Spec;

=head2 $srtm3v2_directory

Point this to the directory that contains the converted SRTM3 version 2 tiles.

You can download the tiles from ftp://e0srp01u.ecs.nasa.gov/srtm/version2/SRTM3
When you get the data from NASA, the tiles are separated by continent and
are in a zip-file format that we can't use directly.  The files will be named
something like Eurasia/30E074.hgt.zip.

Grab the script srtm_generate_hdr.sh from http://mpa.itc.it/rs/srtm/ and run 
it on the files you get from NASA.  This will generate the proper headers for
the files, and convert them from the .hgt.zip format to a .tif.

I converted them with the commands:

  cd $dest_dir
  find $source_dir -name "*.hgt.zip" -exec srtm_generate_hdr.sh {} \;

I recommend un-commenting the line in srtm_generate_hdr.sh that cleans up
the temporary files:
  rm -f $TILE.bil $TILE.hdr $TILE.hgt $TILE.prj

After it is done, you can optionally compress the resulting .tif files with
bzip2 or gzip to save disk space.

$srtm3v2_directory must point to the directory that contains all of 
the (possibly compressed) .tif files.  They should not be separated into 
continent directories.
=cut
#our $srtm3v2_directory = "/usr/local/share/xcsoar/srtm3v2";
our $srtm3v2_directory = "/usr/local/share/xcsoar/srtm_finished";

=head2 $copy_uncompressed_srtm_tiles

The SRTM .tif tiles can be compressed with bzip or gzip to save space.  
When needed, the compressed tiles are copied to a temporary directory
and uncompressed.  If this variable is set to '1', uncompressed tiles 
will be copied to a temporary directory also.  If it's set to '0', 
uncompressed tiles will be used in-place.  This is really only useful
during development if you're paranoid about altering the original data.
=cut

our $copy_uncompressed_srtm_tiles = 1;


=head2 $topology_directory

Point this to the directory where you have untarred the topology files.  
This directory will have the following four subdirectories: v0eur, v0noa, 
v0sas, and v0soa.

Get the topo files from
  * v0noa.tar.gz (257.659597 Mbytes)
     http://geoengine.nima.mil/ftpdir/archive/vpf_data/v0noa.tar.gz
  * v0eur.tar.gz (251.242530 Mbytes) (Europe and Africa)
     http://geoengine.nima.mil/ftpdir/archive/vpf_data/v0eur.tar.gz
  * v0soa.tar.gz (175.975418 Mbytes)
     http://geoengine.nima.mil/ftpdir/archive/vpf_data/v0soa.tar.gz
  * v0sas.tar.gz (239.897712 Mbytes) (Australia)
     http://geoengine.nima.mil/ftpdir/archive/vpf_data/v0sas.tar.gz
  * Total File size: 924.775257 Mbytes

Uncompress the files in $topology_directory.

=cut

our $topology_directory = "/usr/local/share/xcsoar/vmap0";

=head2 $pidfile_dir

directory for the pid file that keeps the terrain generation daemon
from running multiple copies.  Tmpdir should be ok, unless that directory
is prone to being full or frequently cleaned.

=cut
our $xcsoar_var_dir = "/var/xcsoar";
our $pidfile_dir = $xcsoar_var_dir;

=head2 $queue_dir

directory for work requests from the web interface to the terrain daemon

=cut
our $airspace_directory = "/usr/local/share/xcsoar/openair";
our $log_directory = File::Spec->catfile($xcsoar_var_dir, 'log');
our $incoming_queue_dir = File::Spec->catfile($xcsoar_var_dir,
					      'queue');
our $working_queue_dir = File::Spec->catfile($xcsoar_var_dir, 'working');
our $outgoing_queue_dir = File::Spec->catfile($xcsoar_var_dir,'completed');
our $session_directory = File::Spec->catfile($xcsoar_var_dir,'sessions');
our $temp_directory = File::Spec->catfile($xcsoar_var_dir, 'temp');
our $finished_output_directory = File::Spec->catfile($xcsoar_var_dir, 'output');
our $from_email_address = 'terrain-daemon@xcsoar.dd.com.au';
our $html_template_path = 'template';
1;
