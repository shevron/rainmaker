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

#define XML_ATTR_TO_BOOLEAN(v) (xmlStrncasecmp(v, BAD_CAST "yes", 4) == 0 || xmlStrncasecmp(v, BAD_CAST "true", 5) == 0)
#define XML_IF_NODE_NAME(nd, nm) if (xmlStrcmp(nd->name, BAD_CAST nm) == 0)

static gboolean read_request_headers_xml(xmlNode *node, rmRequest *request, GError **error)
{
    xmlXPathContextPtr  xpathCtx;
    xmlXPathObjectPtr   xpath;
    xmlNodeSetPtr       nodes;
    xmlChar            *attr, *value;
    gboolean            replace;
    guint               i;

    const xmlChar *xpathExpr = BAD_CAST "headers/header";

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

        attr = xmlGetProp(nodes->nodeTab[i], BAD_CAST "replace");
        if (attr != NULL) {
            replace = XML_ATTR_TO_BOOLEAN(attr);
            xmlFree(attr);
        } else {
            replace = FALSE;
        }

        attr = xmlGetProp(nodes->nodeTab[i], BAD_CAST "name");
        if (attr == NULL) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "missing required attribute 'name' on header XML element in line %hu", nodes->nodeTab[i]->line);
            xmlXPathFreeContext(xpathCtx);
            xmlXPathFreeObject(xpath);
            return FALSE;
        }

        value = xmlNodeListGetString(node->doc, nodes->nodeTab[i]->children, 1);
        rm_request_add_header(request, (const gchar *) attr, (const gchar *) value, replace);
        xmlFree(attr);
        xmlFree(value);
    }

    xmlXPathFreeContext(xpathCtx);
    xmlXPathFreeObject(xpath);

    return TRUE;
}

gboolean read_request_body_form_data(xmlNode *node, rmRequest *request, GError **error)
{
    return TRUE;
}

gboolean read_request_body_raw_data(xmlNode *node, rmRequest *request, GError **error)
{
    xmlChar  *attr;
    gchar    *body;
    gsize     body_len;
    gboolean  base64, trim;

    g_assert(node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, BAD_CAST "rawData") == 0);

    // Get content
    body = (gchar *) xmlNodeListGetString(node->doc, node->children, 1);
    if (body == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC,
                "error reading raw body value from scenario XML");
        return FALSE;
    }

    // Do we need to somehow manipulate the raw body?
    base64 = FALSE;
    trim   = FALSE;
    if ((attr = xmlGetProp(node, BAD_CAST "base64"))) {
        if (XML_ATTR_TO_BOOLEAN(attr)) {
            base64 = TRUE;
        }
        xmlFree(attr);
    }

    if ((attr = xmlGetProp(node, BAD_CAST "trim"))) {
        if (XML_ATTR_TO_BOOLEAN(attr)) {
            trim = TRUE;
        }
        xmlFree(attr);
    }

    if (base64) {
        body = (gchar *) g_base64_decode_inplace((gchar *) body, &body_len);
        body = g_realloc_n(body, body_len + 1, sizeof(gchar));
    } else if(trim) {
        body = g_strstrip(body);
        body_len = xmlStrlen(BAD_CAST body);
        body = g_realloc_n(body, body_len + 1, sizeof(gchar));

    } else {
        body_len = xmlStrlen(BAD_CAST body);
    }

    // Set the content type
    if ((attr == xmlGetProp(node, BAD_CAST "contentType"))) {
        // rmRequest will free the content type when freed, no need to copy XML string
        request->bodyType = (gchar *) attr;
    } else {
        request->bodyType = g_strdup("application/octet-stream");
    }

    request->body = body;
    request->bodyLength = body_len;
    request->freeBody = TRUE;

    return TRUE;
}

static rmRequest* new_request_from_xml_node(xmlNode *node, rmRequest *baseRequest, GError **error)
{
    rmRequest *req;
    xmlChar   *attr;
    xmlNode   *child;

    g_assert(node->type == XML_ELEMENT_NODE);

    if ((attr = xmlGetProp(node, BAD_CAST "url")) == NULL) {
        // URI property is missing, check base request
        if (baseRequest->url == NULL) {
            // No base URL defined
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "required property 'url' is missing for request");
            return NULL;

        } else {
            // Set request URL to blank string == base URL
            attr = xmlStrdup(BAD_CAST "");
        }
    }

    // Create the request, we will set the method later
    req = rm_request_new(NULL, (gchar *) attr, baseRequest->url, error);
    xmlFree(attr);

    // Set the request method
    if ((attr = xmlGetProp(node, BAD_CAST "method"))) {
        // Method defined for this request, use it
        req->method = g_quark_from_string((gchar *) attr);
        xmlFree(attr);

    } else if (baseRequest->method) {
        // Method not defined but we have method from the baseRequest
        req->method = baseRequest->method;

    } else {
        // We got no method, and no fallback
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
            "required property 'method' is missing for request");
        rm_request_free(req);
        return NULL;
    }

    // Set the repeat count for the request
    if ((attr = xmlGetProp(node, BAD_CAST "repeat")))  {
        req->repeat = atoi((const char *) attr);
        xmlFree(attr);
        if (req->repeat < 1) {
            // We got no method, and no fallback
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "request repeat count must be larger than 0");
            rm_request_free(req);
            return NULL;
        }
    }

    // Iterate over child nodes to handle them
    for (child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE) {
            XML_IF_NODE_NAME(child, "rawData") {
                // Read the raw request body
                if (! read_request_body_raw_data(child, req, error))
                    break;

            } else XML_IF_NODE_NAME(child, "formData") {
                // Read a script element
                if (! read_request_body_form_data(child, req, error))
                    break;

            } else XML_IF_NODE_NAME(child, "headers") {
                // Headers are read later using XPath (FIXME?)

            } else {
                g_printerr("WARNING: unrecognized XML element '%s'\n", child->name);
            }
        }
    }

    if (*error != NULL) {
        rm_request_free(req);
        return NULL;
    }

    // Add base request headers and then read any request-specific headers
    g_slist_foreach(baseRequest->headers, (GFunc) rm_header_copy_to_request, (gpointer) req);
    if (! read_request_headers_xml(node, req, error)) {
        rm_request_free(req);
        return NULL;
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
    prop = xmlGetProp(node, BAD_CAST "method");
    if (prop != NULL) {
        baseRequest->method = g_quark_from_string((gchar *) prop);
        xmlFree(prop);
    }

    // Read base URL
    prop = xmlGetProp(node, BAD_CAST "url");
    if (prop != NULL) {
        baseRequest->url = soup_uri_new((const gchar *) prop);
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

        if (xmlStrcmp(opt->name, BAD_CAST "option") != 0) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Unexpected node '%s', expecting 'option'", opt->name);

            return FALSE;
        }

        if ((attr = xmlGetProp(opt, BAD_CAST "name")) == NULL) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Missing required attribute 'name' in 'option' element");
            return FALSE;
        }

        if ((value = xmlGetProp(opt, BAD_CAST "value")) == NULL) {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "Missing required attribute 'value' in 'option' element");
            xmlFree(attr);
            return FALSE;
        }

        // What option are we setting?
        if (xmlStrcmp(attr, BAD_CAST "persistCookies") == 0) {
            scenario->persistCookies = XML_ATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, BAD_CAST "failOnHttpError") == 0) {
            scenario->failOnHttpError = XML_ATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, BAD_CAST "failOnTcpError") == 0) {
            scenario->failOnTcpError = XML_ATTR_TO_BOOLEAN(value);

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
    if (root_node == NULL || (xmlStrcmp(root_node->name, BAD_CAST "testScenario") != 0)) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
            "unexpected XML root element: '%s', expecting '%s'",
            root_node->name,
            "testScenario");
    } else {
        // Iterate tree and read data into memory
        scenario = rm_scenario_new();
        baseRequest = g_malloc0(sizeof(rmRequest));

        for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                XML_IF_NODE_NAME(cur_node, "request") {
                    // Read a request object
                    if (! read_request_xml_add_req(cur_node, scenario, baseRequest, error))
                        break;

                } else XML_IF_NODE_NAME(cur_node, "script") {
                    // Read a script element
                    if (! read_script_xml(cur_node, scenario, error))
                        break;

                } else XML_IF_NODE_NAME(cur_node, "options") {
                    // Read the 'options' section
                    if (! read_options_xml(cur_node, scenario, error))
                        break;

                } else XML_IF_NODE_NAME(cur_node, "baseRequest") {
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
