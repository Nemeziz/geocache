package GeoCache::Client;

use strict;

use Encode;
use URI::Escape qw(uri_escape_utf8);
use IO::Socket;

sub new {
    my $class = shift;
    my ($host, $port) = @_;
    bless { host => ($host || '127.0.0.1'),
            port => ($port || 1732), } => $class;
}

sub _clean_data {
    for ($_[0]) {
        s[[\r\n]+][ ]sgo;
        s[\A\s*][]o;
        s[\s*\z][]o;
        s[\s+][ ]go;
    }
}

sub send {
    my $self = shift;
    my $query = shift;
    _clean_data($query);
    return if !$query;

    $query = lc $query;

    my $sock = IO::Socket::INET->new(PeerHost => $self->{host},
                                     PeerPort => $self->{port});
    if (!$sock) {
        print STDERR "Cannot connect to geocache: $!\n";
        return;
    }

    print {$sock} uri_escape_utf8($query) . "\n\n";
    my $result = <$sock>;
    $sock->close();
    return wantarray ? ($query, $result) : $result;
}

1;

__END__

=pod

=head1 NAME

GeoCache::Client

=head1 SYNOPSIS

  my $query;
  my $client = GeoCache::Client->new($address, $port);
  my $result = $client->send($query);

=head1 AUTHOR

Yung-chung Lin (henearkrxern@gmail.com)

=cut
