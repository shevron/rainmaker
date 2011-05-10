#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libsoup/soup.h>

#include "rainmaker.h"

#define RM_XML_ERROR g_quark_from_static_string("rainmaker-xml-error")

enum {
    RM_XML_ERROR_ALLOC,
    RM_XML_ERROR_IO,
    RM_XML_ERROR_PARSE,
    RM_XML_ERROR_VALIDATE
};

typedef struct _rmRequestListStruct {
    rmRequest                   *request;
    struct _rmRequestListStruct *next;
} rmRequestList;

typedef struct _rmScenarioSturct {
    rmRequestList *requests;
    gboolean       persistCookies;
    gboolean       failOnHttpError;
    SoupURI       *baseUrl;
    gint           repeat;
} rmScenario;

static rmScenario *rm_scenario_new()
{
    rmScenario *scenario;

    scenario = g_malloc(sizeof(rmScenario));

    // Set some default values
    scenario->requests        = NULL;
    scenario->persistCookies  = FALSE;
    scenario->failOnHttpError = FALSE;
    scenario->baseUrl         = NULL;
    scenario->repeat          = 0;

    return scenario;
}

static void rm_scenario_free(rmScenario *scenario)
{
    rmRequest *req, *nextreq;

    g_assert(scenario != NULL);

    // Free list of requests
    for (req = scenario->requests; req; req = nextreq) {
        rm_request_free(req->request);
        nextreq = req->next;
        g_free(req);
    }

    // Free options if needed
    if (scenario->baseUrl != NULL) g_free(scenario->baseUrl);

    // Finally, free scenario memory
    g_free(scenario);
}

static rmRequest new_request_from_xml_node(xmlNode *node, rmScenario *scenario, GError **error)
{
    rmRequest *req; 
    xmlAttr   *attr;
    xmlChar   *value;

    g_assert(node->type == XML_ELEMENT_NODE);

    // Create request object and set some key attributes
    req = rm_request_new();
    req->method = xmlGetProp(node, "method");
    
    // Set the request URI
    if ((value = xmlGetProp(node, "url")) != NULL) { 
        req->url = soup_uri_new(value);
        xmlFree(value);
    }

    return request;
}

static gboolean scenario_read_request_xml(xmlNode *node, rmScenario *scenario, rmRequest *default_req, GError **error)
{
    return TRUE;
}

static gboolean scenario_read_default_request_xml(xmlNode *node, rmScenario *scenario, rmRequest *default_req, GError **error)
{
    return TRUE;
}

static gboolean scenario_read_script_xml(xmlNode *node, rmScenario *scenario, GError **error)
{
    return TRUE;
}

static gboolean scenario_read_options_xml(xmlNode *node, rmScenario *scenario, GError **error)
{
    return TRUE;
}

static rmScenario *scenario_read_from_xml_stream(FILE *file, GError **error)
{
    xmlParserCtxtPtr  xmlCtx;
    xmlDocPtr         xmlDoc;
    xmlNode          *root_node, *cur_node;
    rmScenario       *scenario    = NULL;
    rmRequest        *default_req = NULL;
    int               filefd      = fileno(file); 

    // Read XML from open stream 
    if ((xmlCtx = xmlNewParserCtxt()) == NULL) { 
        g_set_error(error, RM_XML_ERROR, RM_XML_ERROR_ALLOC, 
            "failed creating XML parser context");
        return NULL;
    }
    
    if ((xmlDoc = xmlCtxtReadFd(xmlCtx, filefd, "/", "utf8", 0)) == NULL) {
        g_set_error(error, RM_XML_ERROR, RM_XML_ERROR_PARSE,
            "failed to parse open XML file");
        return NULL;
    }

    xmlFreeParserCtxt(xmlCtx);

    root_node = xmlDocGetRootElement(xmlDoc);
    if (root_node == NULL || (strcmp(root_node->name, "testScenario") != 0)) {
        g_set_error(error, RM_XML_ERROR, RM_XML_ERROR_VALIDATE, 
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
                    if (! scenario_read_request_xml(cur_node, scenario, default_req, error))
                        break;

                } else IF_NODE_NAME("script") {
                    // Read a script element
                    if (! scenario_read_script_xml(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("options") { 
                    // Read the 'options' section
                    if (! scenario_read_options_xml(cur_node, scenario, error))
                        break;

                } else IF_NODE_NAME("defaultRequest") { 
                    // Read the 'defaultRequest' section
                    if (! scenario_read_default_request_xml(cur_node, scenario, default_req, error))
                        break;

                } else { 
                    g_set_error(error, RM_XML_ERROR, RM_XML_ERROR_VALIDATE, 
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

static rmScenario *scenario_read_from_xml_file(char *filename, GError **error)
{
    rmScenario *scenario; 
    FILE       *fileptr; 

    fileptr = fopen(filename, "r");
    if (! fileptr) { 
        g_set_error(error, RM_XML_ERROR, RM_XML_ERROR_IO, 
            "unable to open scenario file '%s': %s",
            filename,
            strerror(errno));
        return NULL;
    }

    scenario = scenario_read_from_xml_stream(fileptr, error);

    if (! scenario) {
        g_assert(error == NULL || *error != NULL);
    } else {
        g_assert(error == NULL || *error == NULL);
    }

    fclose(fileptr);

    return scenario;
}

int main(int argc, char *argv[])
{
    rmScenario *scenario; 
    GError     *error = NULL;

    if (argc != 2) { 
        fprintf(stderr, "Usage: %s <scenario file>\n", argv[0]);
        return 1;
    }

    scenario = scenario_read_from_xml_file(argv[1], &error);
    if (! scenario) { 
        fprintf(stderr, "ERROR: %s\n", 
            (error != NULL ? error->message : "<unknown error>"));
        return 1;
    }

    rm_scenario_free(scenario);

    return 0;
}

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */

