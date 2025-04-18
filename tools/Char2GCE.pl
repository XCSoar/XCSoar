#!/usr/bin/perl

use strict;
use warnings;

while (<>) {
  if (/GCE_([A-Z0-9_]+)/ && !/GCE_COUNT/) {
    my $v = lc($1);
    print qq'"gce_$v",\n'
  }
}
