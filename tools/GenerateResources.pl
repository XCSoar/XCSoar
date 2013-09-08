#!/usr/bin/perl -w

use strict;

sub generate_blob($$) {
    my ($var, $path) = @_;
    open FILE, "<$path" or die $!;

    print "static constexpr uint8_t $var\[\] = {\n";

    my $data;
    while ((my $nbytes = read(FILE, $data, 64)) > 0) {
        for (my $i = 0; $i < $nbytes; ++$i) {
            printf "0x%02x, ", ord(substr($data, $i, 1));
        }
        print "\n";
    }

    print "};\n";
}

print "#include <stdint.h>\n";

my @numeric;
my @named;

while (<>) {
    # merge adjacent strings
    while (s/"([^"]*)"\s+"([^"]*)"\s*$/"$1$2"/) {}

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

print "#include \"Util/ConstBuffer.hpp\"\n";
print "#include <tchar.h>\n";

print "static constexpr struct {\n";
print "  unsigned id;\n";
print "  ConstBuffer<void> data;\n";
print "} numeric_resources[] = {";
foreach my $i (@numeric) {
    print "  { $i, { resource_$i, sizeof(resource_$i) } },\n";
}
print "  { 0, { nullptr, 0 } }\n";
print "};\n";

print "static constexpr struct {\n";
print "  const TCHAR *name;\n";
print "  ConstBuffer<void> data;\n";
print "} named_resources[] = {";
foreach my $i (@named) {
    my $variable = "resource_$i";
    $variable =~ s,\.,_,g;
    print "  { _T(\"$i\"), { $variable, sizeof($variable) } },\n";
}
print "  { 0, { nullptr, 0 } }\n";
print "};\n";
