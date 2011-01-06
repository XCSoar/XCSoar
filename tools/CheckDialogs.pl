#!/usr/bin/perl

=head1 SYNOPSIS

B<CheckDialogs.pl>

=head1 DESCRIPTION

This program performs some checks on the dialog XML files in
Data/Dialogs/

=cut

use strict;
use warnings;

use vars qw($path $file_id %strings);

use XML::Parser;

sub handle_start {
    my $expat = shift;
    my $element = shift;

    my $control;
    my @x = @_;
    while (scalar @x >= 2) {
        my ($name, $value) = (shift @x, shift @x);
        $control = $value if $name eq 'Name';
    }

    unless (defined $control) {
        if ($element eq 'Form') {
            $control = $element;
        } elsif ($element eq 'Label' or $element eq 'Button') {
            return;
        }
    }

    # hack to allow portrait mode of dlgWaypointEdit
    return if defined $control and
      ($control eq 'prpLongitudeSign' or $control eq 'prpLatitudeSign');

    while (scalar @_ >= 2) {
        my ($name, $value) = (shift, shift);

        if ($name eq 'Caption' or $name eq 'Help') {
            next unless $value =~ /[a-zA-Z]/;

            die "No control name in " . $path . ":" . $expat->current_line . "\n"
              unless defined $control;

            my $other = $strings{$file_id}{$control}{$name};
            die "Difference in $path:$control:$name\n"
              if defined $other and $value ne $other;
            $strings{$file_id}{$control}{$name} = $value;
        }
    }
}

my %handlers = (
    Start => \&handle_start,
);

foreach $path (glob 'Data/Dialogs/*.xml') {
    $path =~ /^(.*?)(_L)?\.xml$/ or die;
    $file_id = $1;

    my $parser = new XML::Parser(Handlers => \%handlers);
    $parser->parsefile($path);
}
