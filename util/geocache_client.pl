#!/usr/bin/perl

use strict;
use FindBin;
use lib "$FindBin::Bin/lib";
use Time::HiRes qw(time);
use GeoCache::Client;


sub main {
    my $host = ($ARGV[0] || '127.0.0.1');
    my $port = ($ARGV[1] || 1732);

    my $client = GeoCache::Client->new($host, $port);

    binmode(STDOUT, ":utf8");
    binmode(STDIN, ":utf8");
    while (1) {
        print "> ";
        my $query = <STDIN>;

        my $time1 = time();
        my $result;
        ($query, $result) = $client->send($query);
        print qq[\n  "$query": $result\n];
        print time() - $time1, " secs.\n\n";
    }
}

main;
