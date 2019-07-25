#ifndef __HTTP_REQUEST_CB_H__
#define __HTTP_REQUEST_CB_H__

#include "http_parser.h"


int http_request_url_cb (http_parser *p, const char *buf, size_t len);

int http_header_field_cb (http_parser *p, const char *buf, size_t len);

int http_header_value_cb (http_parser *p, const char *buf, size_t len);

int http_headers_complete_cb (http_parser *p);

int http_message_begin_cb (http_parser *p);

int http_message_complete_cb (http_parser *p);

int http_body_cb (http_parser *p, const char *buf, size_t len);

int http_response_status_cb (http_parser *p, const char *buf, size_t len);

#endif
