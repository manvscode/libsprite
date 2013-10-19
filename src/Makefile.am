
# Add new files in alphabetical order. Thanks.
libsprite_src = texture-packer.c sprite.c sprite-player.c

# Add new files in alphabetical order. Thanks.
libsprite_headers = texture-packer.h sprite.h

library_includedir      = $(includedir)/libsprite/
library_include_HEADERS = $(libsprite_headers)

# sprite library
lib_LTLIBRARIES                          = $(top_builddir)/lib/libsprite.la 
__top_builddir__lib_libsprite_la_SOURCES = $(libsprite_src)
__top_builddir__lib_libsprite_la_LIBADD  = -lcollections -limageio


