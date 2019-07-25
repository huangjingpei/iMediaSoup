#include "http_server.h"
#include "http_connection.h"
#include "http_request_cb.h"
#include "utility.h"



int http_request_url_cb (http_parser *p, const char *buf, size_t len)
{
    //printf("[%s] %s  %d\n\n\n", __FUNCTION__, dumpbuf,len);
    sp<CHttpConnection>_conn = (CHttpConnection *)p->data;

    _conn->updateURL(buf,len);
    return 0;
}

int http_header_field_cb (http_parser *p, const char *buf, size_t len)
{

    sp<CHttpConnection>_conn = (CHttpConnection *)p->data;

    _conn->updateFieldName(buf,len);
    return 0;
}

int http_header_value_cb (http_parser *p, const char *buf, size_t len)
{
    sp<CHttpConnection>_conn = (CHttpConnection *)p->data;
    _conn->updateFieldValue(buf,len);
    return 0; 
}

int http_headers_complete_cb (http_parser *p)
{
    //printf("[%s] \n\n\n\n",__FUNCTION__);
    sp<CHttpConnection>_conn = (CHttpConnection *)p->data;

    return 0;
}

int http_message_begin_cb (http_parser *p)
{
    //printf("[%s]\n\n\n\n",__FUNCTION__);

    return 0;
}

int http_message_complete_cb (http_parser *p)
{
    //printf("[%s]\n\n\n\n",__FUNCTION__);
    return 0;
}

int http_body_cb (http_parser *p, const char *buf, size_t len)
{
    MARK_LINE;
    //printf("http_body_cb len:%d\n",len);
    sp<CHttpConnection>_conn = (CHttpConnection *)p->data;
   
    const char *pContentType = _conn->getContentType();
    CHttpJsonServer::Instance()->http_server_trigger_cb(_conn,buf,len);
    return 0;
}

int http_response_status_cb (http_parser *p, const char *buf, size_t len)
{
    //printf("[%s]\n\n\n\n",__FUNCTION__);

    return 0;
}
