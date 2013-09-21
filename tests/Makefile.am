#AM_CFLAGS = -std=c99 -Wall -pedantic -O3 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
AM_CFLAGS = -std=c99 -pg -g -ggdb -O0 -I$(top_builddir)/src/ -I/usr/local/include/ -L$(top_builddir)/lib/  -L/usr/local/lib
LDADD     = -lcollections -limageio

bin_PROGRAMS = \
$(top_builddir)/bin/test-texture-packing \
$(top_builddir)/bin/test-sprite \
$(top_builddir)/bin/sprc 

__top_builddir__bin_test_texture_packing_SOURCES = test-texture-packing.c
__top_builddir__bin_test_texture_packing_CFLAGS  = 
__top_builddir__bin_test_texture_packing_LDFLAGS = -lcollections -limageio $(top_builddir)/lib/.libs/libsprite.a

__top_builddir__bin_test_sprite_SOURCES = test-sprite.c
__top_builddir__bin_test_sprite_CFLAGS  = 
__top_builddir__bin_test_sprite_LDFLAGS = -lcollections -limageio -lsimplegl -lGL -lSDL2 -framework OpenGL $(top_builddir)/lib/.libs/libsprite.a

__top_builddir__bin_sprc_SOURCES = sprc.c
__top_builddir__bin_sprc_CFLAGS  = 
__top_builddir__bin_sprc_LDFLAGS = -lutility -lcollections -limageio $(top_builddir)/lib/.libs/libsprite.a
