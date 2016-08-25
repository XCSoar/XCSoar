#!/usr/bin/perl

use strict;
use warnings;

while (<>) {
  if (/NE_([A-Z0-9_]+)/ && !/NE_COUNT/) {
    my $v = lc($1);
    print qq'"ne_$v",\n'
  }
}
