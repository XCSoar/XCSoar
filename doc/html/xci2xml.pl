#!/usr/bin/perl
use strict;
use warnings;

my %rec = ();
my $line = 0;

print q{
<input>

<!-- ... example ..

	<mode name="Blah...">

		<type name="key">

			<record data="APP1" title="Show menu">
				<event name="" misc=""/>
				<label location="1" text="Here it is"/>
			</record>

		</type>

	</mode>


-->
};

my %data = ();
%rec = ();
$rec{event} = [];
while (<>) {
	chomp;
	$line++;
	next if (/^#/);

	if (/^\s*$/) {
		if ($rec{type}) {
			foreach my $m (split(/ /, $rec{mode})) {

				$data{$m} = {} unless (ref($data{$m}));
				$data{$m}{$rec{type}} = [] unless (ref($data{$m}{$rec{type}}));

				push @{$data{$m}{$rec{type}}}, {
					event => [@{$rec{event}}],
					label => $rec{label} || "",
					location => $rec{location} || "",
					data => uc($rec{data}),
					title => $rec{title} || join(" ", reverse @{$rec{event}}),
					notes => $rec{notes},
				};
			}
		}
		%rec = ();
		$rec{event} = [];

	# We don't need the quotes - ignore for now
	} elsif (/^event\s*=\s*"*([^"]*)"*$/) {
		push @{$rec{event}}, $1;
	} elsif (/^([a-z0-9]+)\s*=\s*"*([^"]*)"*$/) {
		$rec{$1} = $2;

	} else {
		print STDERR "Error on $line - $_\n";
	}

}

foreach my $m ('default', sort grep { !/default/ } keys %data) {

	print qq{
		<mode name="$m">
	};

	foreach my $type (sort keys %{$data{$m}}) {
		print qq{
			<type name="$type">
		};

		foreach my $e (@{$data{$m}{$type}}) {
			my $t = xml_entify($e->{title});
			my $l = xml_entify($e->{label});
			print qq[
				<entry data="$e->{data}" title="$t" label="$l" location="$e->{location}">
			];
			foreach my $er (reverse @{$e->{event}}) {
				my ($et, $em) = split(/ /, $er, 2);
				print qq{
					<event name="$et" misc="$em"/>
				};
			}
			print qq{
				</entry>
			};
		}

		print qq{
			</type>
		};
	}

	print qq{
		</mode>
	};

}

print q{
</input>
};


exit 0;
sub xml_entify {
        my ($str, $no_escape_tilde) = @_;
        return undef unless(defined($str));
        $str =~ s/[\x00-\x08\x10\x0B\x0C\x0E-\x1F]//g;
        $str =~ s/([&<>"\x{80}-\x{10ffff}])/"&#".ord($1).";"/eg;
        $str =~ s/~/~~/g unless $no_escape_tilde;
        return $str;
}

