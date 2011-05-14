##
# rainmaker HTTP load generator
# Copyright (c) 2010-2011 Shahar Evron
#
# rainmaker is free / open source software, available under the terms of the
# New BSD License. See COPYING for license details.
##

# rainmaker Makefile.am

bin_PROGRAMS = rmtest

rmtest_SOURCES = rmtest-main.c \
                 rainmaker-client.c \
                 rainmaker-request.c \
                 rainmaker-scenario.c \
                 rainmaker-scenario-xml.c \
                 rainmaker-scoreboard.c

xsdFile = rainmaker-scenario-1.0.xsd
rmsharedir = $(datadir)/$(PACKAGE)
rmshare_DATA = $(xsdFile) 

LIBS = @LIBSOUP_LIBS@
AM_CFLAGS = $(LIBSOUP_CFLAGS) \
            -D RM_XML_XSD_FILE=\"$(xsdFile)\"
            
AM_CPPFLAGS = $(LIBSOUP_CFLAGS)
