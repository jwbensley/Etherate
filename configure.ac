AC_INIT([etherate],[0.10.0],[jwbensley@gmail.com],,[https://null.53bits.co.uk/index.php?page=etherate])

AM_INIT_AUTOMAKE([1.9 tar-pax])

AC_PROG_CXX
AC_HEADER_STDC
AC_CHECK_HEADERS
AC_SYS_LARGEFILE

AC_PROG_LIBTOOL

#CFLAGS="-Wall -D_FILE_OFFSET_BITS=$ac_cv_sys_file_offset_bits"
CFLAGS="-Wall -ftrapv -lrt -D _GNU_SOURCE -DPACKAGE=\\\"\$(PACKAGE)\\\" -DSYSCONFDIR=\\\"\$(sysconfdir)\\\" -DLOCALSTATEDIR=\\\"\$(localstatedir)\\\" -DLIBEXECDIR=\\\"\$(libexecdir)\\\" -DLIBDIR=\\\"\$(libdir)\\\" -DPACKAGE_VERSION=\\\"\$(PACKAGE_VERSION)\\\"" 
CXXFLAGS=$CFLAGS

AC_MSG_CHECKING(whether to enable debugging)
debug_default="no"
AC_ARG_ENABLE(debug, [  --enable-debug=[no/yes] turn on debugging
                       [default=${debug_default}]],, enable_debug=$debug_default)
if test "x$enable_debug" = "xyes"; then
#    AC_DEFINE([DEBUG], [], [Build a Debug Version])
    CFLAGS="$CFLAGS -g -pipe -O0 -DDEBUG"
    CXXFLAGS="$CXXFLAGS -g -pipe -O0 -DDEBUG"
    AC_MSG_RESULT(yes)
    AM_CONDITIONAL(DEBUG, test "x$enable_debug" = "xyes")
else
    CFLAGS="$CFLAGS -O3 -g -pipe"
    CXXFLAGS="$CXXFLAGS -O3 -g -pipe"
    AC_MSG_RESULT(no)
    AM_CONDITIONAL(DEBUG, test "x$enable_debug" = "xyes")
fi

AC_SUBST([DEBUG])
AC_SUBST([VERSION])

# AC_SUBST([datadir])

_BUILD_HOST="`uname -m`"
AM_CONDITIONAL(BUILD_64, test "x$_BUILD_HOST" = "xx86_64")

AC_CONFIG_FILES([Makefile
		])

AC_OUTPUT

