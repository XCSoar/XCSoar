package XCS::Cache;
use warnings;
use strict;
use File::Copy;
use Carp;

=head1 NAME

XCS::Cache - File cache, don't downlad that thing twice !

=head1 SYNOPSIS

	use XCS::Cache;
	my $tpc = XCS::Cache->new("/tmp/tpc");

	unless ($tpc->test("location.jpg")) {

		# Write data structure
		$tpc->write("location.jpg", $data);

		# OR copy file
		copyfile("original.jpg", $tpc->filename);

	}

=cut

sub new {
	my ($class, $dir) = @_;

	my $self = bless {}, ref($class) || $class;
	$self->{DIR} = $dir;
	return undef unless ($self->_checkdir);
	return $self;
}

=head2 test(entry)

Does the file exist ?

=cut

sub test {
	my ($self, $entry) = @_;
	return (-f $self->_fullname($entry));
}

=head2 read(entry)

Return array/string of file data

=cut

sub read {
	my ($self, $entry) = @_;
	return "" unless ($self->test($entry));
	my $fh;
	open ($fh, $self->_fullname($entry));
	my @buffer = <$fh>;
	# XXX Could break binary data !
	return wantarray ? @buffer : join("\n", @buffer);
}

=head2 write(entry, data)

Write string/array to file

=cut

sub write {
	my ($self, $entry, @data) = @_;
	my $fh;
	open ($fh, ">" . $self->_fullname($entry));
	print $fh join("\n", @data);
	close $fh;
}

=head2 filename(entry)

Return full file name

=cut

sub filename {
	my ($self, $entry) = @_;
	return $self->_fullname($entry);
}

=head2 copyfile_to(entry, new_file_name)

copy self entry to a new filename

=cut

sub copyfile_to {
	my ($self, $entry, $new) = @_;
	copy($self->_fullname($entry), $new);
}

=head2 copyfile_from(entry, old_file_name)

Copy an existing file to self entry

=cut

sub copyfile_from {
	my ($self, $entry, $old) = @_;
	copy($old, $self->_fullname($entry));
}

# Really return full name, test etc
sub _fullname {
	my ($self, $entry) = @_;
	my $ret = $self->{DIR};
	$ret .= "/" unless (substr($ret, 0, -1) eq "/");

	if ($entry =~ m|/|) {
		croak "Invalid entry - $entry - must not contain /";
	}

	$ret .= $entry;
}

# Make directory, check it then exists.
sub _checkdir {
	my ($self) = @_;

	unless (-d $self->{DIR}) {
		mkdir $self->{DIR};
	}

	return (-d $self->{DIR});
}

1;

