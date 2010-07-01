#!/usr/bin/perl -w

use strict;

sub generate_blob($$) {
    my ($var, $path) = @_;
    open FILE, "<$path" or die $!;

    print "static const unsigned char $var\[\] = {\n";

    my $data;
    while ((my $nbytes = read(FILE, $data, 64)) > 0) {
        for (my $i = 0; $i < $nbytes; ++$i) {
            printf "0x%02x, ", ord(substr($data, $i, 1));
        }
        print "\n";
    }

    print "};\n";
}

my @numeric;

while (<>) {
    if (/^\s*(\d+)\s+BITMAP\s+DISCARDABLE\s+"(.*?)"\s*$/) {
        push @numeric, $1;
        generate_blob("resource_$1", "Data/$2");
    }
}

print "#include <stddef.h>\n";

print "static const struct {\n";
print "  unsigned id;\n";
print "  const unsigned char *data;\n";
print "  size_t size;\n";
print "} numeric_resources[] = {";
foreach my $i (@numeric) {
    print "  { $i, resource_$i, sizeof(resource_$i) },\n";
}
print "  { 0, NULL, 0 }\n";
print "};\n";
