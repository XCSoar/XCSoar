#!/usr/bin/perl

=head1 SYNOPSIS

B<xml2po.pl> I<inputfiles.xml>

=head1 DESCRIPTION

This program extracts translatable strings from dialog XML files
(Data/Dialogs/*.xml) and prints them to stdout in the gettext PO
format.

=cut

use strict;
use warnings;

use vars qw($path %strings);

use XML::Parser;

sub handle_start {
    my $expat = shift;
    my $element = shift;

    while (scalar @_ >= 2) {
        my ($name, $value) = (shift, shift);

        if ($name eq 'Caption' or $name eq 'Help') {
            next unless $value =~ /[a-zA-Z]/;

            die "Malformed attribute at $path:" . $expat->current_line . "\n  -> \"" . $value . "\"\n"
              if $value =~ /^\s|\s$|:$|\t| \n| \r|\n /s;

            $value =~ s,\*$,,s;

            $strings{$value} ||= [];
            push @{$strings{$value}}, $path . ":" . $expat->current_line;
        }
    }
}

my %handlers = (
    Start => \&handle_start,
);

foreach $path (@ARGV) {
    my $parser = new XML::Parser(Handlers => \%handlers, ErrorContext => 2);
    $parser->parsefile($path);
}

sub quote($) {
    my $_ = shift;
    s,\\,\\\\,g;
    s,",\\",g;
    s,\n,\\n,g;
    s,\r,\\r,g;
    return $_;
}

foreach my $value (keys %strings) {
    print "#: @{[join(' ', @{$strings{$value}})]}\n";
    print "msgid \"@{[quote $value]}\"\n";
    print "msgstr \"\"\n";
    print "\n";
}
