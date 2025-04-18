#!/usr/bin/perl -w

use strict;

sub generate_blob($) {
    my ($var) = @_;

    print "extern const std::byte ${var}\[\];\n";
    print "extern const std::byte ${var}_end\[\];\n";
}

print "#include <cstddef>\n";
print "#include <span>\n";

my @named;

while (<>) {
    next if /^\s*(?:#.*)?$/;

    if (/^(?:app_icon|bitmap_bitmap|bitmap_graphic|hatch_bitmap|bitmap_icon_scaled)\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        # only sounds used here
    } elsif (/^sound\s+([\w_]+)\s+"([^"]+)"\s*$/) {
        push @named, [ $1, -s "output/data/sound/$2.raw" ];
        my $variable = "resource_$1";
        $variable =~ s,\.,_,g;
        generate_blob($variable);
    } else {
        die "Syntax error: $_";
    }
}

print "#include <tchar.h>\n";

print "static constexpr struct {\n";
print "  const TCHAR *name;\n";
print "  std::span<const std::byte> data;\n";
print "} named_resources[] = {";
foreach my $i (@named) {
    my ($name, $size) = @$i;
    my $variable = "resource_${name}";
    $variable =~ s,\.,_,g;
    print "  { _T(\"${name}\"), { ${variable}, ${size} } },\n";
}
print "  { 0, {} }\n";
print "};\n";
