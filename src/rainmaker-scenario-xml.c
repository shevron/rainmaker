/// ---------------------------------------------------------------------------
/// Rainmaker HTTP load testing tool
/// Copyright (c) 2010-2011 Shahar Evron
///
/// Rainmaker is free / open source software, available under the terms of the
/// New BSD License. See COPYING for license details.
/// ---------------------------------------------------------------------------

#include <string.h>
#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>
#include <glib.h>

#include "rainmaker-request.h"
#include "rainmaker-scenario.h"
#include "rainmaker-scenario-xml.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef RM_DATA_DIR
#define RM_DATA_DIR "./"
#endif

#ifndef RM_XML_XSD_FILE
#define RM_XML_XSD_FILE "rainmaker-scenario-1.0.xsd"
#endif

#ifndef RM_XML_NS
#define RM_XML_NS "http://arr.gr/rainmaker/xmlns/scenario/1.0"
#endif

#define XML_ATTR_TO_BOOLEAN(v) (xmlStrcmp(v, BAD_CAST "yes") == 0 || xmlStrcmp(v, BAD_CAST "true") == 0)
#define XML_IF_NODE_NAME(nd, nm) if (xmlStrcmp(nd->name, BAD_CAST nm) == 0)

/// Read request headers from XML tree. This will use XPath to grab both the
/// default client headers as well as the request specific headers and set them
static gboolean read_request_headers_xml(xmlNode *node, rmRequest *request, GError **error)
{
    xmlXPathContextPtr  xpathCtx;
    xmlXPathObjectPtr   xpath;
    xmlNodeSetPtr       nodes;
    xmlChar            *attr, *value;
    gboolean            replace;
    guint               i;

    const xmlChar *xpathExpr = BAD_CAST "//rm:clientSetup/rm:headers/rm:header | rm:headers/rm:header";

    g_assert(node->type == XML_ELEMENT_NODE);

    // Use xPath to grab all "header" nodes
    xpathCtx = xmlXPathNewContext(node->doc);
    if (xpathCtx == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC,
            "unable to allocate XPath context");
        return FALSE;
    }
    xpathCtx->node = node;

    // Register XML namespace
    if(xmlXPathRegisterNs(xpathCtx, BAD_CAST "rm", BAD_CAST RM_XML_NS) != 0) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC,
            "unable to register XML namespace '%s' in XPath context", RM_XML_NS);
        xmlXPathFreeContext(xpathCtx);
        return FALSE;
    }

    xpath = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    if (xpath == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_ALLOC,
            "unable to evaluate XPath expression '%s'", xpathExpr);
        xmlXPathFreeContext(xpathCtx);
        return FALSE;
    }

    nodes = xpath->nodesetval;
    for (i = 0; nodes && i < nodes->nodeNr; i++) {
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

static gboolean read_request_body_form_data(xmlNode *node, rmRequest *request, GError **error)
{
    GSList         *params = NULL;
    rmRequestParam *param;
    xmlChar        *attr, *value;
    xmlNode        *child;

    g_assert(node->type == XML_ELEMENT_NODE);
    g_assert(xmlStrcmp(node->name, BAD_CAST "formData") == 0);

    // Figure out encoding type (default is form-urlencoded)
    attr = xmlGetProp(node, BAD_CAST "enctype");
    if (attr != NULL) {
        request->bodyType = g_quark_from_string((const gchar *) attr);
        xmlFree(attr);
    } else {
        request->bodyType = g_quark_from_static_string("application/x-www-form-urlencoded");
    }

    // Iterate over child elements and add them to the param list
    for (child = node->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) continue;
        XML_IF_NODE_NAME(child, "formParam") {
            // Get param name
            attr = xmlGetProp(child, BAD_CAST "name");
            if (attr == NULL) {
                g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                        "missing required XML attribute 'name' for 'formParam' element in line %u", child->line);
                break;
            }

            // Get param value
            value = xmlNodeListGetString(child->doc, child->children, 1);
            if (value == NULL) {
                g_printerr("Warning: form parameter '%s' (defined in line %u) has no value\n",
                        attr, child->line);
                value = xmlStrdup(BAD_CAST "");
            }

            // TODO: for now all params are strings, we should support other (complex) types
            param = rm_request_param_new(RM_REQUEST_PARAM_STRING);
            param->name = (gchar *) attr;
            param->strValue = (gchar *) value;

            params = g_slist_append(params, (gpointer) param);

        } else {
            g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                    "unexpected XML element '%s', expecting 'formParam' in line %u", child->name, child->line);
            break;
        }
    }

    if (*error != NULL) {
        rm_gslist_free_full(params, (GDestroyNotify) rm_request_param_free);
        return FALSE;
    }

    // Encode parameters as request body
    request->body = rm_request_encode_params(params, request->bodyType, &request->bodyLength, error);
    rm_gslist_free_full(params, (GDestroyNotify) rm_request_param_free);
    if (request->body == NULL) {
        return FALSE;
    }

    return TRUE;
}

static gboolean read_request_body_raw_data(xmlNode *node, rmRequest *request, GError **error)
{
    xmlChar  *attr;
    gchar    *body;
    gsize     body_len;
    gboolean  base64, trim;

    g_assert(node->type == XML_ELEMENT_NODE);
    g_assert(xmlStrcmp(node->name, BAD_CAST "rawData") == 0);

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
    attr = xmlGetProp(node, BAD_CAST "contentType");
    if (attr != NULL) {
        // rmRequest will free the content type when freed, no need to copy XML string
        request->bodyType = g_quark_from_string((const gchar *) attr);
    } else {
        request->bodyType = g_quark_from_static_string("application/octet-stream");
    }

    request->body = body;
    request->bodyLength = body_len;
    request->freeBody = TRUE;

    return TRUE;
}

static rmRequest* new_request_from_xml_node(xmlNode *node, const SoupURI *baseUrl, GError **error)
{
    rmRequest *req;
    xmlChar   *attr;
    xmlNode   *child;

    g_assert(node->type == XML_ELEMENT_NODE);

    if ((attr = xmlGetProp(node, BAD_CAST "url")) == NULL) {
        // URI property is missing, check base request
        if (baseUrl == NULL) {
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
    req = rm_request_new(NULL, (gchar *) attr, baseUrl, error);
    xmlFree(attr);
    if (req == NULL) {
        return NULL;
    }

    // Set the request method
    attr = xmlGetProp(node, BAD_CAST "method");
    if (attr != NULL) {
        // Method defined for this request, use it
        req->method = g_quark_from_string((gchar *) attr);
        xmlFree(attr);

    } else {
        // We got no method, assume GET
        req->method = g_quark_from_static_string("GET");
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
    if (! read_request_headers_xml(node, req, error)) {
        rm_request_free(req);
        return NULL;
    }

    return req;
}

static gboolean read_request_xml_add_req(xmlNode *node, rmScenario *scenario, const SoupURI *baseUrl, GError **error)
{
    rmRequest *req;

    req = new_request_from_xml_node(node, baseUrl, error);
    if (! req) {
        return FALSE;
    }

    rm_scenario_add_request(scenario, req);

    return TRUE;
}

static gboolean read_script_xml(xmlNode *node, rmScenario *scenario, GError **error)
{
    g_assert(node->type == XML_ELEMENT_NODE);

    return TRUE;
}

/// Read the 'options' XML element
static gboolean read_options_xml(xmlNode *node, rmScenario *scenario, SoupURI **baseUrl, GError **error)
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

        } else if (xmlStrcmp(attr, BAD_CAST "failOnHttpRedirect") == 0) {
            scenario->failOnHttpRedirect = XML_ATTR_TO_BOOLEAN(value);

        } else if (xmlStrcmp(attr, BAD_CAST "baseUrl") == 0) {
            g_assert(*baseUrl == NULL);
            *baseUrl = soup_uri_new((const char *) value);

        } else {
            // Unknown option
            g_printerr("WARNING: unknown option '%s'\n", attr);
        }

        xmlFree(attr);
        xmlFree(value);
    }

    return TRUE;
}

/// Read the 'clientSetup' XML element. In the case of this element, we are only
/// interested in the 'options' child element of it, as the 'headers' child
/// element is read later using XPath
static gboolean read_client_setup_xml(xmlNode *node, rmScenario *scenario, SoupURI **baseUrl, GError **error)
{
    xmlNode *child;

    g_assert(node->type == XML_ELEMENT_NODE);
    g_assert(xmlStrcmp(node->name, BAD_CAST "clientSetup") == 0);

    // Iterate over child nodes to handle them
    for (child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE) {
            XML_IF_NODE_NAME(child, "options") {
                if (! read_options_xml(child, scenario, baseUrl, error)) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

/// Validate the input XML file using an XML Schema file
static gboolean validate_scenario_xml(const xmlDocPtr doc, GError **error)
{
    xmlDocPtr              schemaDoc  = NULL;
    xmlSchemaParserCtxtPtr parserCtxt = NULL;
    xmlSchemaPtr           schema     = NULL;
    xmlSchemaValidCtxtPtr  validCtxt  = NULL;
    int                    status;

    schemaDoc = xmlReadFile(RM_DATA_DIR RM_XML_XSD_FILE, NULL, XML_PARSE_NONET);
    if (schemaDoc == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "error reading XML schema file '%s'", RM_DATA_DIR RM_XML_XSD_FILE);
        goto returnWithError;
    }

    parserCtxt = xmlSchemaNewDocParserCtxt(schemaDoc);
    if (parserCtxt == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "unable to create XML parser context from schema doc");
        goto returnWithError;
    }

    schema = xmlSchemaParse(parserCtxt);
    if (schema == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "XML schema '%s' is invalid", RM_DATA_DIR RM_XML_XSD_FILE);
        goto returnWithError;
    }

    validCtxt = xmlSchemaNewValidCtxt(schema);
    if (validCtxt == NULL) {
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "unable to create XML schema validation context from schema doc");
        goto returnWithError;
    }

    // Validate document
    status = xmlSchemaValidateDoc(validCtxt, doc);
    if (status < 0) {
        // Internal parser error
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "an internal schema validator error has occurred");
        goto returnWithError;
    } else if (status > 0) {
        // Validation error
        g_set_error(error, RM_ERROR_XML, RM_ERROR_XML_VALIDATE,
                "the provided scenario file is not valid");
        goto returnWithError;
    }

    xmlFreeDoc(schemaDoc);
    xmlSchemaFreeParserCtxt(parserCtxt);
    xmlSchemaFree(schema);
    xmlSchemaFreeValidCtxt(validCtxt);

    return TRUE;

returnWithError:
    if (schemaDoc != NULL)  xmlFreeDoc(schemaDoc);
    if (parserCtxt != NULL) xmlSchemaFreeParserCtxt(parserCtxt);
    if (schema != NULL)     xmlSchemaFree(schema);
    if (validCtxt != NULL)  xmlSchemaFreeValidCtxt(validCtxt);

    return FALSE;
}

/// Read a scenario XML file from an open stream and return a scenario struct
static rmScenario *read_scenario_from_xml_stream(FILE *file, GError **error)
{
    xmlParserCtxtPtr  xmlCtx;
    xmlDocPtr         xmlDoc;
    xmlNode          *root_node, *cur_node;
    rmScenario       *scenario    = NULL;
    SoupURI          *baseUrl     = NULL;

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

    // Validate XML according to schema
    if (validate_scenario_xml(xmlDoc, error)) {
        // Iterate tree and read data into memory
        root_node = xmlDocGetRootElement(xmlDoc);
        scenario = rm_scenario_new();

        for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                XML_IF_NODE_NAME(cur_node, "request") {
                    // Read a request object
                    if (! read_request_xml_add_req(cur_node, scenario, baseUrl, error))
                        break;

                } else XML_IF_NODE_NAME(cur_node, "script") {
                    // Read a script element
                    if (! read_script_xml(cur_node, scenario, error))
                        break;

                } else XML_IF_NODE_NAME(cur_node, "clientSetup") {
                    // Read the 'options' section
                    if (! read_client_setup_xml(cur_node, scenario, &baseUrl, error))
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

    if (baseUrl != NULL) soup_uri_free(baseUrl);
    xmlFreeDoc(xmlDoc);
    return scenario;
}

/// Read scenario from XML file and return a new scenario struct
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

// vim:ts=4:expandtab:cindent:sw=2
