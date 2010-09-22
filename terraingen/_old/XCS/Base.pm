package XCS::Base;
use warnings;
use strict;
use LWP::UserAgent;

sub init { }

sub get_url {
        my ($self, $url, $referer) = @_;

        # XXX Once off, no need for continual changing/creating
        my $ua = LWP::UserAgent->new;
        $ua->agent("Mozilla/5.001 (Macintosh; N; PPC; ja) Gecko/25250101");

        # Create a request
        my $req = HTTP::Request->new(GET => $url);
        $req->referer($referer) if ($referer);

        # XXX other options - such as login/password
        # XXX Posting support etc.

        # Pass request to the user agent and get a response back
        my $res = $ua->request($req);

        # Check the outcome of the response
        if ($res->is_success) {
                return $res->content;
                #Better, but breaks binary data (problem is that params are arrays)
                #return wantarray ? split(/[\r\n]/, $res->content) : $res->content;
        } else {
                # XXX Better dealing with errors
                croak $res->status_line, "\n";
        }

}

sub debug {
	my ($self, $msg) = @_;
	print STDERR "DEBUG: $msg\n";
}

sub verbose {
	my ($self, $msg) = @_;
	print $msg . "\n";
}

sub download_delay {
	sleep 1;
}

1;
