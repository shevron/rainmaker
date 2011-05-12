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
#include <libxml/xpath.h>
#include <glib.h>

#include "rainmaker-request.h"
#include "rainmaker-scenario.h"
#include "rainmaker-scenario-xml.h"

#define XMLATTR_TO_BOOLEAN(v) (xmlStrncasecmp(v, "yes", 4) == 0 || xmlStrncasecmp(v, "true", 5) == 0)

gboolean read_request_headers_xml(xmlNode *node, rmRequest *request, GError **error)
{
    xmlXPathContextPtr  xpathCtx;
    xmlXPathObjectPtr   xpath;
    xmlNodeSetPtr       nodes;
    xmlChar            *attr, *value;
    gboolean            replace;
    guint               i;

    const xmlChar *xpathExpr =  "headers/header";

    g_assert(node->type == XML_ELEMENT_NODE);

    // Use xPath to grab all "header" nodes
    xpathCtx = xmlXPathNewContext(node->doc);
    if (xpathCtx == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC,
            "unable to allocate XPath context");
        return FALSE;
    }
    xpathCtx->node = node;

    xpath = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    if (xpath == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC,
            "unable to evaluate XPath expression '%s'", xpathExpr);
        xmlXPathFreeContext(xpathCtx);
        return FALSE;
    }

    nodes = xpath->nodesetval;
    for (i = 0; i < nodes->nodeNr; i++) {
        g_assert(nodes->nodeTab[i]);

        attr = xmlGetProp(nodes->nodeTab[i], "replace");
        if (attr != NULL) {
            replace = XMLATTR_TO_BOOLEAN(attr);
            xmlFree(attr);
        } else {
            replace = FALSE;
        }

        attr = xmlGetProp(nodes->nodeTab[i], "name");
        if (attr == NULL) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "missing required attribute 'name' on header XML element in line %hu", nodes->nodeTab[i]->line);
            xmlXPathFreeContext(xpathCtx);
            xmlXPathFreeObject(xpath);
            return FALSE;
        }

        value = xmlNodeListGetString(node->doc, nodes->nodeTab[i]->children, 1);
        rm_request_add_header(request, attr, value, replace);
        xmlFree(attr);
        xmlFree(value);
    }

    xmlXPathFreeContext(xpathCtx);
    xmlXPathFreeObject(xpath);

    return TRUE;
}

static rmRequest* new_request_from_xml_node(xmlNode *node, rmRequest *baseRequest, GError **error)
{
    rmRequest *req;
    xmlChar   *attr;
    gboolean   free_method = FALSE;

    g_assert(node->type == XML_ELEMENT_NODE);

    if ((attr = xmlGetProp(node, "url")) == NULL) {
        // URI property is missing, check base request
        if (baseRequest->url == NULL) {
            // No base URL defined
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "required property 'uri' is missing for request");
            return NULL;

        } else {
            // Set request URL to blank string == base URL
            attr = xmlStrdup("");
        }
    }

    // Create the request, we will set the method later
    req = rm_request_new(NULL, attr, baseRequest->url, error);
    xmlFree(attr);

    // Set the request method
    if ((attr = xmlGetProp(node, "method"))) {
        // Method defined for this request, use it
        req->method = g_quark_from_string(attr);
        xmlFree(attr);

    } else if (baseRequest->method) {
        // Method not defined but we have method from the baseRequest
        req->method = baseRequest->method;

    } else {
        // We got no method, and no fallback
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
            "required property 'method' is missing for request");
        rm_request_free(req);
        return FALSE;
    }

    // Add base request headers and then read any request-specific headers
    g_slist_foreach(baseRequest->headers, (GFunc) rm_header_copy_to_request, (gpointer) req);
    if (! read_request_headers_xml(node, req, error)) {
        rm_request_free(req);
        return FALSE;
    }

    return req;
}

static gboolean read_request_xml_add_req(xmlNode *node, rmScenario *scenario, rmRequest *baseRequest, GError **error)
{
    rmRequest *req;

    req = new_request_from_xml_node(node, baseRequest, error);
    if (! req) {
        return FALSE;
    }

    rm_scenario_add_request(scenario, req);

    return TRUE;
}

static gboolean read_base_request_xml(xmlNode *node, rmRequest *baseRequest, GError **error)
{
    xmlChar *prop;

    g_assert(node->type == XML_ELEMENT_NODE);

    // Read base method
    prop = xmlGetProp(node, "method");
    if (prop != NULL) {
        baseRequest->method = g_quark_from_string(prop);
        xmlFree(prop);
    }

    // Read base URL
    prop = xmlGetProp(node, "url");
    if (prop != NULL) {
        baseRequest->url = soup_uri_new(prop);
        if (! SOUP_URI_VALID_FOR_HTTP(baseRequest->url)) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Provided base URL '%s' is not a valid, full HTTP URL", prop);
            xmlFree(prop);
            return FALSE;
        }
        xmlFree(prop);
    }

    // Read headers, if any
    if (! read_request_headers_xml(node, baseRequest, error))
        return FALSE;

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

        // What option are we setting?
        if (xmlStrcmp(attr, "persistCookies") == 0) {
            scenario->persistCookies = XMLATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, "failOnHttpError") == 0) {
            scenario->failOnHttpError = XMLATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, "failOnTcpError") == 0) {
            scenario->failOnTcpError = XMLATTR_TO_BOOLEAN(value);

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
    rmRequest        *baseRequest = NULL;

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
        baseRequest = g_malloc0(sizeof(rmRequest));

#define IF_NODE_NAME(n) if (strcmp(cur_node->name, n) == 0)

        for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                IF_NODE_NAME("request") {
                    // Read a request object
                    if (! read_request_xml_add_req(cur_node, scenario, baseRequest, error))
                        break;

                } else IF_NODE_NAME("script") {
                    // Read a script element
                    if (! read_script_xml(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("options") {
                    // Read the 'options' section
                    if (! read_options_xml(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("baseRequest") {
                    // Read the 'defaultRequest' section
                    if (! read_base_request_xml(cur_node, baseRequest, error))
                        break;

                } else {
                    g_printerr("WARNING: unrecognized XML element '%s'\n", cur_node->name);
                }
            }
        }
    }

    if (*error != NULL && scenario != NULL) {
        rm_scenario_free(scenario);
        scenario = NULL;
    }

    if (baseRequest != NULL) rm_request_free(baseRequest);
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
