#!/usr/bin/perl

use strict;
use warnings;

while (<>) {
    print qq'{ "$1", &InputEvents::event$1 },\n'
      if /^\s*void event([a-zA-Z0-9]+)/;
}
