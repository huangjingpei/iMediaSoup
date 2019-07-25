#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>
#include "cJSON.h"
#include "http_parser.h"
#include <ev.h>
#include "ABase.h"
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <sys/time.h>
#include "Poller.h"
#include "Transport.h"

#define MAX_SERVICEID_LEN  (512)
#define MAX_CONF_STR_LEN  (512)


using namespace android;


class CHttpConnection : public ITransport
{
public:
    static status_t create(sp<CHttpConnection> &inst);
    virtual ~CHttpConnection();


    int init(int _fd, int debug_level);
    int start(struct ev_loop *loop);
    int start();
    int sendReq(cJSON *root);
    void Reply(char *szResp);

public:
    void updateURL(char *_url, int len);
    void updateFieldName(char *_name, int len);
    void updateFieldValue(char *_value, int len);
    void dumpHttpHeadInfo();
    const char *getContentType();
    bool ConnInit(const char *pszServiceID);
    const char* GetServiceID(){
        return mServiceID;
    }
private:
    REFLECT_READ_HANDLER(CHttpConnection, mHandler, handleRead);

    virtual void handleClose(){}
    virtual void handleFree(){}
    virtual void handleWrite(){}
    virtual void resetSocket() {}


    //static void connection_cb(struct ev_loop *loop, ev_io *w, int revents);

    void close_connection(struct ev_loop *loop, ev_io *w);
    void close_connection();

    void parse_http_content_len(char **buf);
    int send_response(char *response) ;
    
    int get_request_len(const char *buf, size_t buflen);
    char * skip(char **buf, const char *delimiters);
    void releaseResource();

    //int send_result( cJSON * result,cJSON * id);
    //int create_result(cJSON * result,cJSON * id, cJSON **result_root) ;
    //int send_error(int code, char* message,cJSON * id) ;
    //int create_error(int code, char* message,cJSON * id,cJSON **result_root ) ;
    
private:
    int                     m_fd;
    //ev_io                   m_conn_io;    
    int                     m_pos;
    unsigned int            m_buffer_size;
    char *                  m_buffer;
    int                     m_debug_level;
    http_parser             m_parser;
    int                     m_request_len;
    int                     content_len;
    char                    field_name[255];
    char                    resp_buf[BUFSIZ];
    char                    http_resp[BUFSIZ];

    char                    *url;
    int                     num_headers;        /* Number of headers    */
    struct mg_header {
        char    *name;      /* HTTP header name */
        char    *value;     /* HTTP header value    */
    } http_headers[64];     /* Maximum 64 headers   */

    char                    mServiceID[MAX_SERVICEID_LEN];
private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(CHttpConnection);
};


#endif

