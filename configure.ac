AC_PREREQ(2.67)
AC_INIT([libsprite], [0.1], [manvscode@gmail.com], [libsprite], [http://www.manvscode.com/])
AM_INIT_AUTOMAKE([1.13 foreign silent-rules -Wall -Werror])

AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([src/libsprite-config.h])

AC_PROG_CC
AC_PROG_CC_C99
AC_C_INLINE
AC_HEADER_STDC


AH_TOP([
#ifndef _LIBSPRITE_H_
#define _LIBSPRITE_H_
])
AH_BOTTOM([
#endif /* _LIBSPRITE_H_ */
])


#DFLAGS="-D DEBUG_SPRITE -D DEBUG_TEXTURE_PACKER"
DFLAGS="$DFLAGS"

#CFLAGS="-std=gnu99 -Wall -pedantic -g -ggdb -O0 $DFLAGS -I/usr/local/include"
CFLAGS="$CFLAGS"
LDFLAGS="$LDFLAGS -lutility -lcollections"

AM_PROG_AR

LT_INIT([shared static])
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL


AC_ARG_ENABLE([programs],
	[AS_HELP_STRING([--disable-programs], [Enable command line tools.])],
	[:],
	[enable_programs=no])

AM_CONDITIONAL([ENABLE_PROGRAMS], [test "$enable_programs" = "yes"])


AC_PROG_INSTALL

echo " ------------------------------ "
echo ""
echo "  CFLAGS = $CFLAGS"
echo " LDFLAGS = $LDFLAGS"
echo ""
echo " ------------------------------ "

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	tests/Makefile
])

AC_OUTPUT
