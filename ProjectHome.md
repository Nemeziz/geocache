Geocache is a standalone proxy server storing geocoding results from Google Maps. This may save some of the daily quota of geocoding requests to Google Maps.

What it does is extremely simple. It forwards your geocoding queries to google and caches results in a local database, and then sends the results to you. If cache is hit, then results are loaded from the local database. Results are written in CSV format.

Examples:
```

   "台北"     -> geocache -> 200,3,25.085320,121.561490
   "Taipei"   -> geocache -> 200,3,25.085320,121.561490

   "Москва"  -> geocache -> 200,4,55.755786,37.617630
   "Moscow"   -> geocache -> 200,4,55.755786,37.617630

   "東京"     -> geocache -> 200,4,35.668554,139.824310
   "Tokyo"    -> geocache -> 200,4,35.668554,139.824310

   "München"  -> geocache -> 200,4,48.139126,11.580210
   "Munich"   -> geocache -> 200,4,48.139126,11.580210

```

Please download the latest release [here](http://geocache.googlecode.com/files/geocache-0.1.0.tar.gz).

Characters in input queries must be escaped. Please see also [this script](http://geocache.googlecode.com/svn/trunk/util/geocache_client.pl) for reference.

[BerkeleyDB](http://www.oracle.com/database/berkeley-db.html) is required.
