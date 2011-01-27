#!/usr/bin/perl

my $find = $ARGV[0];
my $fname = $ARGV[1];
print "# $find, $fname\n";
open(INFILE,"<$fname");
while (<INFILE>) {
  if (/(.*) # $find/) {
    print "$1\n";
  } else {
    if (/# $find/) {
      print "\n";
    }
  }
}
close(INFILE);
