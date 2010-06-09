package XCS::C302;
use strict;
use warnings;
use Carp;

use base qw/XCS::Base/;
use XCS::C302::Entry;

=head1 NAME

XCS::C302 - Read and Write Cambridge 302 files

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 METHODS

=cut

sub new {
	my ($class, @rest) = @_;
	my $self = bless {}, ref($class) || $class;
	$self->init(@rest);
	return $self;
}

sub init { }

=head2 parseFile

=head2 parseString

=head2 parseURL

=head2 parseAny

Parse the input File or String or URL or Any guess at a file/string/url.

=cut

sub parseFile {
	my ($self, $file) = @_;
	my $fh;
	croak("File does not exist - $file") unless (-f $file);

	open ($fh, $file) || croak("Can't open file $file - $!");
	my @process = ();
	while (<$fh>) {
		chomp;
		push @process, $_;
	}
	return $self->_parse(@process);
}

sub parseString {
	my ($self, $string) = @_;
	return $self->_parse(split(/[\r\n]/, ref $string ? $$string : $string));
}

sub parseURL {
	my ($self, $url) = @_;
	return $self->_parse(split(/[\r\n]/, $self->get_url($url)));
}

sub parseAny {
	my ($self, $entry) = @_;
	if (-f $entry) {
		return $self->parseFile($entry);
	} elsif ($entry =~ /^[a-z]+:.+$/) {
		return $self->parseURL($entry);
	} else {
		return $self->parseString($entry);
	}
}

sub _parse {
	my ($self, @data) = @_;

	$self->{DATA} = [];

	my $linenum = 0;
	foreach my $row (@data) {
		next if ($row =~ /^.*\*/);
		next if ($row =~ /^\s*$/);
		next if ($row =~ /[\x00-\x08\x10\x0B\x0C\x0E-\x1F]/);	# XXX Check this is valid (I think so)
		my ($num, $y, $x, $z, $flag, $title, $other) = split(/\s*,\s*/, $row);
		$linenum++;
		# EMF: this next check seems way too strict.  Many of the
		# waypoint files from the turnpoint exchange skip blocks
		# of numbers, and I don't see any evidence in XCSoar that
		# these numbers need to be contiguous.
		# die "INVALID WAYPOINT FILE - $num in line does not match position of line = $linenum" unless ($linenum == $num);
		push @{$self->{DATA}}, XCS::C302::Entry->new(
			$self,
			{
				'id' => $num,
			        'index' => $linenum,
				'x' => $x,
				'y' => $y,
				'z' => $z,
				'flag' => $flag,
				'title' => $title,
				'other' => $other,
			}
		);
	}
}

=head2 entry(num)

Return a single entry (if it exists0

=cut

sub entry {
	my ($self, $entry) = @_;
	$entry = int($entry * 1);
	if (($entry < 1) || ($entry > $self->count)) {
		croak "Invalid entry $entry";
	}
	return $self->{DATA}[$entry - 1];
}

=head2 count

Number of entries - which always start at 1 (Note, start 1, not 0)

=cut

sub count {
	return scalar(@{$_[0]->{DATA}});
}

=head2 entries

All entries...

=cut

sub entries {
	my ($self) = @_;
	return @{$self->{DATA}};
}

=head2 copy_to

Copy the file to an output - even if it is not a file.
Note - that for safety, may want to just keep a copy of the file/string/url as
parsing it...

=cut

sub copy_to {
	my ($self, $out) = @_;
	my $fh;
	# XXX Add in headers, Ctrl-Z at end etc...
	# 	(better still, find someone else that has already done it)
	open ($fh, ">$out") || croak("Unable to create file $out - $!");
	print $fh $self->generate;
	close $fh;
}

=head2 generate

Generate a file content.

=cut

sub generate {
	my ($self) = @_;
	my @buffer = ();
	foreach my $wp ($self->entries) {
		push @buffer, $wp->generate();
	}
	return join("\n", @buffer);
	# return wantarray ? @buffer : join("\n", @buffer);
}


sub x_min {
	my ($self) = @_;

	my $x = $self->entry(1)->x_norm;
	foreach my $wp ($self->entries) {
		if ($wp->x_norm < $x) {
			$x = $wp->x_norm;
		}
	}
	return $x;
}

sub x_max {
	my ($self) = @_;
	my $x = $self->entry(1)->x_norm;
	foreach my $wp ($self->entries) {
		if ($wp->x_norm > $x) {
			$x = $wp->x_norm;
		}
	}
	return $x;
}

sub y_min {
	my ($self) = @_;
	my $y = $self->entry(1)->y_norm;
	foreach my $wp ($self->entries) {
		if ($wp->y_norm < $y) {
			$y = $wp->y_norm;
		}
	}
	return $y;
}

sub y_max {
	my ($self) = @_;
	my $y = $self->entry(1)->y_norm;
	foreach my $wp ($self->entries) {
		if ($wp->y_norm > $y) {
			$y = $wp->y_norm;
		}
	}
	return $y;
}

1;
