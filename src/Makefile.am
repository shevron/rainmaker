##
# rainmaker HTTP load generator
# Copyright (c) 2010 Shahar Evron
#
# rainmaker is free / open source software, available under the terms of the
# New BSD License. See COPYING for license details.
##

# rainmaker Makefile.am

bin_PROGRAMS = rainmaker

rainmaker_SOURCES = main.c      \
                    rm-client.c \
                    rainmaker.h

rainmaker_LDADD = $(LIBSOUP_LIBS)

AM_CFLAGS = $(LIBSOUP_CFLAGS)

AM_CPPFLAGS = $(LIBSOUP_CFLAGS)

