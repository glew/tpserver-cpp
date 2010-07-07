## Process this file with automake to produce Makefile.in

persistencelibdir = $(libdir)/tpserver/persistence

if SQLITE
persistencelib_LTLIBRARIES = libtpsqlite.la
else
persistencelib_LTLIBRARIES =
endif

libtpsqlite_la_SOURCES = sqlitepersistence.h sqlitepersistence.cpp

libtpsqlite_la_LIBADD = @SQLITE_LIBS@
libtpsqlite_la_LDFLAGS = -export-dynamic -module
AM_CXXFLAGS = @SQLITE_CFLAGS@

EXTRA_DIST = sample.conf
