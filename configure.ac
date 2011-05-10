dnl --- 
dnl rainmaker HTTP load generator
dnl Copyright (c) 2010 Shahar Evron
dnl 
dnl rainmaker is free / open source software, available under the terms of the
dnl New BSD License. See COPYING for license details.
dnl ---

# Paocess this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([rainmaker], [0.1], [shahar@arr.gr])
AC_CONFIG_SRCDIR([src/rainmaker-request.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([LIBGLIB], [glib-2.0 >= 2.8])
PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4 >= 2.24])
PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.7])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([soup-2.4], [soup_cookie_jar_new], [
  AC_DEFINE(HAVE_LIBSOUP_COOKIEJAR, 1, [define if libsoup supports SoupCookieJar])
], [ 
  AC_WARN([libsoup is too old, cookie persistance support is disabled])
], [$LIBSOUP_LIBS])

AC_SUBST(LIBSOUP_CFLAGS)
AC_SUBST(LIBSOUP_LIBS)

AC_CONFIG_FILES([Makefile
		 src/Makefile])

AC_OUTPUT
