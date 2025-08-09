AC_INIT([volmix], [0.1.0], [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])

# Check for required libraries
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0.0])
PKG_CHECK_MODULES([PULSE], [libpulse >= 0.9.16])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32.0])

# Define paths for data files
AC_SUBST(pkgdatadir, ['${datadir}/volmix'])

AC_OUTPUT