#!/usr/bin/perl
use warnings;
use strict;

print q{<?xml version="1.0" encoding="UTF-8"?>
<document>
        <metadata>
                <title>FAQ</title>
        </metadata>

        <content>

	<h1>FAQ</h1>
};


chdir "faq/";
open (IN, "ls *.faq |");
while (<IN>) {
	chomp;
	s/\.faq//;
	my $in = $_;

	print qq{
		<faq href="$in.html" faqref="$in.faq"/>
	};

}

print q{
	</content>
</document>
};


