#! /usr/bin/perl -w
use lib qw(/var/www/www.xcsoar.com/cgi-bin);
use lib qw(.);
use XCS::TerrainGenerator;
my $app = new XCS::TerrainGenerator;
$app->run();
