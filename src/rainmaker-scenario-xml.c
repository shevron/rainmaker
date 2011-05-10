/**
 * Rainmaker HTTP load testing tool
 * Copyright (c) 2010-2011 Shahar Evron
 *
 * Rainmaker is free / open source software, available under the terms of the
 * New BSD License. See COPYING for license details.
 */

#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <glib.h>

#include "rainmaker-request.h"
#include "rainmaker-scenario.h"
#include "rainmaker-scenario-xml.h"

/* {{{ static rmRequest* new_request_from_xml_node(xmlNode *node, rmScenario *scenario, GError **error)
 * 
 * Create a new request struct from XML node
 */
static rmRequest* new_request_from_xml_node(xmlNode *node, rmScenario *scenario, GError **error)
{
    rmRequest *req; 
    xmlChar   *method, *url;

    g_assert(node->type == XML_ELEMENT_NODE);

    if ((method = xmlGetProp(node, "method")) == NULL) { 
        // Method property is missing
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
            "required property 'method' is missing for request");
        return NULL;
    } 
   
    // Set the request URI
    if ((url = xmlGetProp(node, "url")) == NULL) { 
        // URI property is missing
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
            "required property 'uri' is missing for request");
        xmlFree(method);
        return NULL;
    }

    // Create request object and set values
    req = rm_request_new(g_strdup(method), url, scenario->baseUrl, error);
    if (req) {
        // Tell request it owns the method string and should free it when done
        req->freeMethod = TRUE;
    }
    
    xmlFree(method);
    xmlFree(url);
 
    return req;
}
/* new_request_from_xml_node }}} */

/* {{{ static gboolean read_request_xml_add_req(xmlNode *node, rmScenario *scenario, GError **error)
 *
 * Add a request to the scenario from XML node
 */
static gboolean read_request_xml_add_req(xmlNode *node, rmScenario *scenario, GError **error)
{
    rmRequest *req;

    req = new_request_from_xml_node(node, scenario, error);
    if (! req) { 
        return FALSE;
    }

    rm_scenario_add_request(scenario, req);

    return TRUE;
}
/* read_request_xml_add_req }}} */

static gboolean read_default_request_xml(xmlNode *node, rmRequest *default_req, GError **error)
{
    return TRUE;
}

static gboolean read_script_xml(xmlNode *node, rmScenario *scenario, GError **error)
{
    g_assert(node->type == XML_ELEMENT_NODE);

    return TRUE;
}

static gboolean read_options_xml(xmlNode *node, rmScenario *scenario, GError **error)
{
    xmlNode *opt; 
    xmlChar *attr, *value;

    g_assert(node->type == XML_ELEMENT_NODE);

    for (opt = node->children; opt; opt = opt->next) { 
        if (opt->type != XML_ELEMENT_NODE) continue;

        if (xmlStrcmp(opt->name, "option") != 0) { 
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE, 
                "Unexpected node '%s', expecting 'option'", opt->name);

            return FALSE;
        }

        if ((attr = xmlGetProp(opt, "name")) == NULL) { 
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Missing required attribute 'name' in 'option' element");
            return FALSE;
        }

        if ((value = xmlGetProp(opt, "value")) == NULL) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Missing required attribute 'value' in 'option' element");
            xmlFree(attr);
            return FALSE;
        }

#define XMLATTR_TO_BOOLEAN(v) (xmlStrncasecmp(v, "yes", 4) == 0 || xmlStrncasecmp(v, "true", 5) == 0)

        // What option are we setting?
        if (xmlStrcmp(attr, "persistCookies") == 0) { 
            scenario->persistCookies = XMLATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, "failOnHttpError") == 0) { 
            scenario->failOnHttpError = XMLATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, "failOnTcpError") == 0) { 
            scenario->failOnTcpError = XMLATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, "baseUrl") == 0) { 
            scenario->baseUrl = soup_uri_new(value);

        } else {
            // Unknown option
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Unknown option: '%s'", attr);
            xmlFree(attr);
            xmlFree(value);
            return FALSE;
        }

        xmlFree(attr);
        xmlFree(value);
    }

    return TRUE;
}

/* {{{ static rmScenario *read_scenario_from_xml_stream(FILE *file, GError **error)
 *
 * Read a scenario XML file from an open stream and return a scenario struct
 */
static rmScenario *read_scenario_from_xml_stream(FILE *file, GError **error)
{
    xmlParserCtxtPtr  xmlCtx;
    xmlDocPtr         xmlDoc;
    xmlNode          *root_node, *cur_node;
    rmScenario       *scenario    = NULL;
    rmRequest        *default_req = NULL;

    // Read XML from open stream 
    if ((xmlCtx = xmlNewParserCtxt()) == NULL) { 
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC, 
            "failed creating XML parser context");
        return NULL;
    }
    
    if ((xmlDoc = xmlCtxtReadFd(xmlCtx, fileno(file), "/", "utf8", 0)) == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_PARSE,
            "failed to parse open XML file");
        return NULL;
    }

    xmlFreeParserCtxt(xmlCtx);

    root_node = xmlDocGetRootElement(xmlDoc);
    if (root_node == NULL || (strcmp(root_node->name, "testScenario") != 0)) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE, 
            "unexpected XML root element: '%s', expecting '%s'",
            root_node->name,
            "testScenario");
    } else {
        // Iterate tree and read data into memory
        scenario = rm_scenario_new();

#define IF_NODE_NAME(n) if (strcmp(cur_node->name, n) == 0)

        for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) { 
            if (cur_node->type == XML_ELEMENT_NODE) { 
                IF_NODE_NAME("request") { 
                    // Read a request object
                    if (! read_request_xml_add_req(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("script") {
                    // Read a script element
                    if (! read_script_xml(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("options") { 
                    // Read the 'options' section
                    if (! read_options_xml(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("defaultRequest") { 
                    // Read the 'defaultRequest' section
                    if (! read_default_request_xml(cur_node, default_req, error))
                        break;

                } else { 
                    g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE, 
                        "unexpected XML element: '%s'", cur_node->name);
                    break;
                }
            }
        }
    }

    if (*error != NULL && scenario != NULL) {
        rm_scenario_free(scenario);
        scenario = NULL;
    }

    if (default_req != NULL) rm_request_free(default_req);
    xmlFreeDoc(xmlDoc);
    return scenario;
}
/* read_scenario_from_xml_stream }}} */

/* {{{ rmScenario *rm_scenario_xml_read_file(char *filename, GError **error)
 *
 * Read scenario from XML file and return a new scenario struct
 */
rmScenario *rm_scenario_xml_read_file(char *filename, GError **error)
{
    rmScenario *scenario; 
    FILE       *fileptr; 

    fileptr = fopen(filename, "r");
    if (! fileptr) { 
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_IO, 
            "unable to open scenario file '%s': %s",
            filename,
            strerror(errno));
        return NULL;
    }

    scenario = read_scenario_from_xml_stream(fileptr, error);

    if (! scenario) {
        g_assert(error == NULL || *error != NULL);
    } else {
        g_assert(error == NULL || *error == NULL);
    }

    fclose(fileptr);

    return scenario;
}
/* rm_scenario_xml_read_file }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
