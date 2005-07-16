#!/usr/bin/perl
use warnings;
use strict;

my $title = shift || "unknown";

print qq{<?xml version="1.0" encoding="UTF-8"?>
<document>
<metadata>
	<title>$title</title>
</metadata>
<content>
<h2>$title</h2>
<pre>
};

while (<>) {
	print xml_entify($_);
}
print q{
</pre>
</content>
</document>
};
exit 0;

sub xml_entify {
        my ($str, $no_escape_tilde) = @_;
	return undef unless(defined($str));
        my $test = $str . chr(2422) . "";
        $test = substr($test, 0, length($test) - 1) . "";

        $test =~ s/[\x00-\x08\x10\x0B\x0C\x0E-\x1F]//g;
        # NOTE - Is it necessary to entify UniCode
        #       Should not be requried, definitely not for Perl 5.8
        $test =~ s/([&<>"\x{80}-\x{10ffff}])/"&#".ord($1).";"/eg;
        $test =~ s/~/~~/g unless $no_escape_tilde;
        return $test;
}

