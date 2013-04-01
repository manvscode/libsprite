#AM_CFLAGS = -std=c99 -Wall -pedantic -O3 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
AM_CFLAGS = -std=c99 -pg -g -ggdb -O0 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
LDADD     = -lsprite -lcollections -lGL -lglut

bin_PROGRAMS = \
$(top_builddir)/bin/test-texture-packing 

__top_builddir__bin_test_texture_packing_SOURCES = test-texture-packing.c
