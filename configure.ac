dnl --- 
dnl rainmaker HTTP load generator
dnl Copyright (c) 2010-2011 Shahar Evron
dnl 
dnl rainmaker is free / open source software, available under the terms of the
dnl New BSD License. See COPYING for license details.
dnl ---

# Paocess this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([rainmaker], [0.1], [shahar@arr.gr])
AC_CONFIG_SRCDIR([src/rainmaker-request.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([glib],    [glib-2.0 >= 2.24])
PKG_CHECK_MODULES([libsoup], [libsoup-2.4 >= 2.24])
PKG_CHECK_MODULES([libxml2], [libxml-2.0 >= 2.6])

# Checks for library functions.
AC_CHECK_LIB([glib-2.0], [g_slist_free_full], [
  AC_DEFINE(HAVE_GLIB_SLIST_FREE_FULL, 1, [define if glib has g_slist_free_full])
], [ ], [$glib_LIBS])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h])

# Define the location of the share dir
AC_DEFINE_DIR([RM_DATA_DIR], [{datadir}/$PACKAGE_NAME/], [location of shared data files])

AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT
