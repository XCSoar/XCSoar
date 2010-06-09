package XCS::C302::Entry;

sub new {
	my ($class, $parent, $data) = @_;
	return bless {
		parent => $parent,
		data => $data,
	}, ref($class) || $class;
}

# XXX Write ?



sub id { return shift->_data('id', @_); }
sub index { return shift->_data('index', @_); }
sub x { return shift->_data('x', @_); }
sub y { return shift->_data('y', @_); }
sub z { return shift->_data('z', @_); }
sub flag { return shift->_data('flag', @_); }
sub title { return shift->_data('title', @_); }
sub other { return shift->_data('other', @_); }

sub _data {
	my ($self, $key, $val) = @_;
	$self->{data}{$key} = $val if (defined($val));
	return $self->{data}{$key};
}

sub _norm {
	my ($self, $orig) = @_;
#THIS LINE IS BUGGED, CHANGE THE LIMIT PARAMETER
	my ($left, $right) = split(/:/, $orig, 2);

	# Reverse ?
	$right =~ s/([NSEW])//;
	if (($1 eq "S") || ($1 eq "W")) {
		$left *= -1;
		$right *= -1;
	}

	# Rounding ? Shorter ?
	return $left + ($right / 60);
}

sub x_norm {
	my ($self) = @_;
	return $self->_norm($self->x);
}

sub y_norm {
	my ($self) = @_;
	return $self->_norm($self->y);
}

# XXX Title stripping etc.

sub generate {
	my ($self) = @_;
	# XXX Note this is not spacing the title as per original - need to ???.
	return join(",", 
		$self->id,
		$self->y,
		$self->x,
		$self->z,
		$self->flag,
		$self->title,
		$self->other
	);
}

1;

