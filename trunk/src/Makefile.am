noinst_HEADERS = gc_conn.h \
	gc_db.h \
	gc_debug.h \
	gc_error.h \
	gc_log.h \
	gc_server.h \
	gc_util.h


bin_PROGRAMS = geocache

geocache_SOURCES = gc_util.c gc_db.c gc_conn.c gc_server.c gc_main.c
geocache_LDADD = $(LDADD) -ldb

clean-local:
	-rm -rf *~ geocache.*
