#!/usr/bin/perl
use strict;

my $CLEAN = ""; my $DEST = ""; my $GEN = "";
foreach my $ext (qw/xml gif png jpg css js xci faq/) {
	#print STDERR "Finiding $ext\n";
	open (IN, "find . -name '*.$ext' | ") || die "Can't open files ($ext) - $!";
	while (<IN>) {
		chomp;
		next if (/old/);
		next if (/tmp/);
		#print STDERR "\t$_\n";
		if ("faq " =~ /$ext /) {
			my $tmp = $_;
			$tmp =~ s/faq$/xml/;
			$GEN .= $tmp . " ";
		}
		if ("xml faq " =~ /$ext /) {
			s/$ext$/html/;
			$CLEAN .= $_ . " ";
		}
		$DEST .= $_ . " ";
	}
	close IN;
}

print "FILES_DEST=$DEST\n\n";
print "FILES_CLEAN=$CLEAN\n\n";
print "FILES_GEN=$GEN\n\n";

