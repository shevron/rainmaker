#include <glib.h>
#include <libsoup/soup.h>

#include "rainmaker-request.h"

/* {{{ rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl) 
 * 
 * Create a new request struct with a defined URL
 */
rmRequest* rm_request_new(const gchar *method, gchar *url, SoupURI *baseUrl)
{
    rmRequest *req;

    req = g_malloc(sizeof(rmRequest));
    
    if (baseUrl == NULL) {
        req->url = soup_uri_new(url);
    } else {
        req->url = soup_uri_new_with_base(baseUrl, url);
    }

    req->method     = method;
    req->headers    = soup_message_headers_new(SOUP_MESSAGE_HEADERS_REQUEST);
    req->body       = NULL;
    req->bodyLength = 0;

    return req;
}
/* rm_request_new }}} */

/* {{{ void rm_request_free(rmRequest *req)
 *
 * Free a request struct and all related memory
 */
void rm_request_free(rmRequest *req)
{
    soup_uri_free(req->url);
    soup_message_headers_free(req->headers);

    g_free(req);
}
/* rm_request_free }}} */

/** 
 * vim:ts=4:expandtab:cindent:sw=2:foldmethod=marker
 */
