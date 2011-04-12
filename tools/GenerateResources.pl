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
my @named;

while (<>) {
    if (/^\s*(\d+)\s+BITMAP\s+DISCARDABLE\s+"(.*?)"\s*$/) {
        push @numeric, $1;
        generate_blob("resource_$1", "Data/$2");
    } elsif (/^\s*([.\w]+)\s+(?:TEXT|XMLDIALOG|MO|RASTERDATA)\s+DISCARDABLE\s+"(.*?)"\s*$/) {
        push @named, $1;
        my $path = $2;
        my $variable = "resource_$1";
        $variable =~ s,\.,_,g;
        generate_blob($variable, "Data/$path");
    }
}

print "#include <stddef.h>\n";
print "#include <tchar.h>\n";

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

print "static const struct {\n";
print "  const TCHAR *name;\n";
print "  const void *data;\n";
print "  size_t size;\n";
print "} named_resources[] = {";
foreach my $i (@named) {
    my $variable = "resource_$i";
    $variable =~ s,\.,_,g;
    print "  { _T(\"$i\"), $variable, sizeof($variable) },\n";
}
print "  { 0, NULL, 0 }\n";
print "};\n";
