#!/usr/bin/perl

my $find = $ARGV[0];
print "# $find\n";
open(INFILE,"<$ARGV[1]");
while (<INFILE>) {
  if (/(.*) # $find/) {
    print "$1\n";
  } else {
    if (/$find/) {
      print "\n";
    }
  }
}
close(INFILE);
