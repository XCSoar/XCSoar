#!/usr/bin/perl

=head1 SYNOPSIS

B<svgl10n.pl> I<-l po-file> I<-o output-file> I<inputfiles.svg>

=head1 DESCRIPTION

This program applys translated strings read from a xx.po file
to creats a new svg file with replaced strings.

=cut

use strict;
use warnings;
use Getopt::Std;
use Locale::PO;

my $po = new Locale::PO();

my %opts = ();
getopts("l:o:", \%opts);
$opts{o} = "-" unless defined $opts{o};

# Read in the po file
if ( not defined $opts{l} ) { die("No \"po\" file given.\n"); }
if ( not -e $opts{l} ) { die("There is no file $opts{l}.\n"); }
my $href = Locale::PO->load_file_ashash($opts{l});

# File to process
my $file = $ARGV[0];
if ( not defined $file ) { die("No file given to process.\n"); }
open (IN, "< " . $file)
    or die("Unable to open input file " . $file . " - $!");

# Output file
open (OUT, "> " . $opts{o})
    or die("Unable to open output " . $opts{o} . " - $!");

foreach my $line (<IN>) {
    if ($line =~ /\>([\w\s\d\=\(\)\/\.]+)\<\/tspan\>/) {
        my $en_text = $po->quote($1);
        if (defined $$href{$en_text}) {
            my $trans = $po->dequote($$href{$en_text}->msgstr());
            $line =~ s/\>([\w\s\d\=\(\)\/\.]+)\</\>$trans\</;
        }
    }
    print OUT $line;
}

# Dump the po file
#foreach my $k (sort keys $href) {
#    print "$k => ", $$href{$k}->msgstr(), "\n";
#}

close IN;
close OUT;
