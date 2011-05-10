/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#ifndef _HAVE_RAINMAKER_SCENARIO_XML_H

#include <glib.h>

#define RM_ERROR_XML g_quark_from_static_string("rainmaker-xml-error")

enum {
    RM_ERROR_XML_ALLOC,
    RM_ERROR_XML_IO,
    RM_ERROR_XML_PARSE,
    RM_ERROR_XML_VALIDATE
};

rmScenario *rm_scenario_xml_read_file(char *filename, GError **error);

#define _HAVE_RAINMAKER_SCENARIO_XML_H
#endif

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
