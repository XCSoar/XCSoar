#!/usr/bin/perl -w

use strict;

print <<EOT;
static const struct {
  unsigned id;
  const char *name;
} DrawableNames[] = {
EOT

while (<>) {
    # merge adjacent strings
    while (s/"([^"]*)"\s+"([^"]*)"\s*$/"$1$2"/) {}

    print qq|  { $1, \"$2\" },\n|
      if /^\s*(\d+)\s+BITMAP\s+DISCARDABLE\s+"(?:.*\/)?([\w]+)\.bmp"\s*$/;
}

print <<EOT;
  { 0, NULL }
};
EOT
