#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

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
	rmRequest     *lastRequest;
	gboolean       persistCookies;
	gint           repeat;
} rmScenario;

static rmScenario *rm_scenario_new()
{
	rmScenario *scenario;

	scenario = g_malloc0(sizeof(rmScenario));
	return scenario;
}

static void rm_scenario_free(rmScenario *scenario)
{
	g_assert(scenario != NULL);
	g_free(scenario);
}

static rmScenario *scenario_read_from_xml_stream(FILE *file, GError **error)
{
	xmlParserCtxtPtr  xmlCtx;
	xmlDocPtr         xmlDoc;
	xmlNode          *root_node, *cur_node;
	rmScenario       *scenario = NULL;
	rmRequest        *default_req = NULL;
	int               filefd = fileno(file); 

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
		goto returnonerror;
	}

	// Iterate tree and read data into memory
	scenario = rm_scenario_new();

#define IF_NODE_NAME(n) if (strcmp(cur_node->name, n) == 0)

	for (cur_node = root_node->children; cur_node; cur_node = cur_node->next) { 
		if (cur_node->type == XML_ELEMENT_NODE) { 
			IF_NODE_NAME("request") { 
				printf("request element handling\n");

			} else IF_NODE_NAME("inputVar") { 
				printf("inputVar element handling\n");

			} else IF_NODE_NAME("script") { 
				printf("script element handling\n");

			} else IF_NODE_NAME("options") { 
				printf("options element handling\n");

			} else IF_NODE_NAME("defaultRequest") { 
				printf("defaultRequest element handling\n");

			} else { 
				g_set_error(error, RM_XML_ERROR, RM_XML_ERROR_VALIDATE, 
					"unexpected XML element: '%s'", cur_node->name);
				goto returnonerror;
			}
		}
	}

	xmlFreeDoc(xmlDoc);
	return scenario;

returnonerror:
	xmlFreeDoc(xmlDoc);
	if (scenario != NULL) rm_scenario_free(scenario);
	return NULL;
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
