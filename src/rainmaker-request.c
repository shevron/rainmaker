#include <glib.h>
#include <libsoup/soup.h>

#include "rainmaker-request.h"

/* {{{ rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error) 
 * 
 * Create a new request struct with a defined URL
 */
rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl, GError **error)
{
    rmRequest *req;

    req = g_malloc(sizeof(rmRequest));
    
    req->headers    = soup_message_headers_new(SOUP_MESSAGE_HEADERS_REQUEST);
    req->method     = method;
    req->body       = NULL;
    req->bodyLength = 0;
    
    if (baseUrl == NULL) {
        req->url = soup_uri_new(url);
    } else {
        req->url = soup_uri_new_with_base(baseUrl, url);
    }

    if (! SOUP_URI_VALID_FOR_HTTP(req->url)) { 
    	g_set_error(error, RM_ERROR_REQ, RM_ERROR_REQ_INVALID_URI, 
		"invalid URI provided for request: '%s'. Are you missing a base URL?", url);
	rm_request_free(req);
	return NULL;
    }

    return req;
}
/* rm_request_new }}} */

/* {{{ void rm_request_free(rmRequest *req)
 *
 * Free a request struct and all related memory
 */
void rm_request_free(rmRequest *req)
{
    if (req->url != NULL) soup_uri_free(req->url);
    soup_message_headers_free(req->headers);

    g_free(req);
}
/* rm_request_free }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
