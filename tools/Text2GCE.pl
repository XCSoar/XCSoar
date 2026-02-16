#!/usr/bin/perl

use strict;
use warnings;

while (<>) {
    print qq'"$1",\n'
      if /GCE_([A-Z0-9_]+)/;
}
