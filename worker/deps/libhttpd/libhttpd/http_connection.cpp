#include "http_server.h"
#include "http_connection.h"
#include "utility.h"

#ifndef GS_LOG_TAG
#define GS_LOG_TAG "CHttpConnection"
#endif

#define   INIT_BUFFER_SIZE   (4096)

#define alloc_cpy(dest, src, len) \
    dest = (char *)malloc(len + 1);\
    memcpy(dest, src, len);\
    dest[len] = '\0';

CHttpConnection::CHttpConnection():
                mHandler(new NetHandler(this)),
                url(NULL),
                num_headers(0)
{
    memset(http_headers,0x00, sizeof(http_headers));
    memset(mServiceID,0x00,sizeof(mServiceID));
}

CHttpConnection::~CHttpConnection()
{
    releaseResource();
}

void CHttpConnection::releaseResource()
{
    if (url) {
        free(url);
        url = NULL;
    }

    for (int i=0; i<num_headers;i++) {
        if (http_headers[i].name) {
            free(http_headers[i].name);
            http_headers[i].name = NULL;
        }
        if (http_headers[i].value) {
            free(http_headers[i].value);
            http_headers[i].value = NULL;
        }
    }

    num_headers = 0;

    m_request_len = 0;
    content_len = 0;
}
void CHttpConnection::updateURL(char *_url, int len)
{
    if (len > 0) {
        alloc_cpy(url,_url,len);
    }
}

void CHttpConnection::updateFieldName(char *_name, int len)
{
    if (len > 0 && num_headers<=64) {
        num_headers++;
        alloc_cpy(http_headers[num_headers-1].name,_name,len);
    }
}

void CHttpConnection::updateFieldValue(char *_value, int len)
{
    if (len > 0 && num_headers<=64) {
        alloc_cpy(http_headers[num_headers-1].value,_value,len);
    }
}

const char *CHttpConnection::getContentType()
{
    for (int i=0; i<num_headers;i++) {
        if (!strcmp(http_headers[i].name,"Content-Type")) {
            return http_headers[i].value;
        }
    }
    return NULL;
}

void CHttpConnection::dumpHttpHeadInfo()
{
    LOGI("%s %s HTTP/%d.%d",http_method_str(m_parser.method),url,m_parser.http_major,m_parser.http_minor);

    for (int i=0; i<num_headers;i++) {
        LOGI("%s:%s",http_headers[i].name,http_headers[i].value);
    }
}
status_t CHttpConnection::create(sp<CHttpConnection> &inst)
{

    status_t err = UNKNOWN_ERROR;
    CHttpConnection *_inst = NULL;
    do
    {
        _inst = new CHttpConnection();
        BREAK_IF(_inst == NULL);

        inst = _inst;

        err = NO_ERROR;
    }while(0);

    if (err != NO_ERROR) {
        delete _inst;
    }

    return err;
}

int CHttpConnection::init(int _fd, int debug_level )
{
    m_fd = _fd;
    //m_conn_io.data = this;
    m_buffer_size = INIT_BUFFER_SIZE;
    m_buffer = (char *)malloc(INIT_BUFFER_SIZE);
    memset(m_buffer, 0, INIT_BUFFER_SIZE);
    m_pos = 0;
    m_request_len = 0;
    content_len = 0;
    m_debug_level = debug_level;
    m_parser.data = this;

    return 0;
}
int CHttpConnection::start(struct ev_loop *loop)
{

    //ev_io_init(&m_conn_io, connection_cb,m_fd, EV_READ);

    http_parser_init(&m_parser, HTTP_BOTH);

    //ev_io_start(loop, &m_conn_io);

    return 0;
}


/// Reflected method
void CHttpConnection::handleRead()
{

    sp<CHttpJsonServer> server = CHttpJsonServer::Instance();
    size_t bytes_read = 0;

    int fd = m_fd;

    if (m_pos == (m_buffer_size - 1)) {
        char * new_buffer = (char *)realloc(m_buffer, m_buffer_size *= 2);
        if (new_buffer == NULL) {
            perror("Memory error");
            return close_connection();
        }
        LOGI("%s realloc buffer_size:%d",__FILE__, m_buffer_size);
        m_buffer = new_buffer;
        memset(m_buffer + m_pos, 0, m_buffer_size - m_pos);
    }
    // can not fill the entire buffer, string must be NULL terminated
    int max_read_size = m_buffer_size - m_pos - 1;
    if ((bytes_read = read(fd, m_buffer + m_pos, max_read_size))
            == -1) {
        LOGI("%s %d %s ,so close connection",__FILE__, errno, strerror(errno));
        return close_connection();
    }

    if (!bytes_read) {
        // client closed the sending half of the connection

        LOGI("%s read return 0, so close connection",__FILE__);
        if (m_pos) {
            size_t parsed =  http_parser_execute(&m_parser, &CHttpJsonServer::m_parser_settings, m_buffer, m_pos);

            if(HPE_OK == HTTP_PARSER_ERRNO(&m_parser)){
                m_pos -= parsed;
                if (!http_should_keep_alive(&m_parser)) {
                    return close_connection();
                }else{
                    releaseResource();
                    return;
                }
            }else{
                LOGI("%s http_parser_execute failed %d %s",__FILE__, HTTP_PARSER_ERRNO(&m_parser),http_errno_name(HTTP_PARSER_ERRNO(&m_parser)));
            }
        }
        return close_connection();
    } else {
        cJSON *root;
        char *end_ptr = NULL;
        m_pos += bytes_read;


         int count = 0;
done:
        if (m_request_len <= 0) {
            m_request_len = get_request_len(m_buffer, (size_t)m_pos);
            if (m_request_len > 0) {
                LOGI("%s request_len:%d  bytes_read:%d buffer_size:%d  %d",__FILE__, m_request_len,bytes_read,m_buffer_size,count);
                count++;
            }else{
                LOGI("%s request_len:%d bytes_read:%d buffer_size:%d",__FILE__, m_request_len,bytes_read,m_buffer_size);
            }
        }
        if (m_pos >= m_request_len && m_request_len>0 && !content_len) {
            parse_http_content_len(&m_buffer);
        }

        if(m_request_len > 0 && content_len && m_pos >= (m_request_len + content_len)){

            size_t parsed =  http_parser_execute(&m_parser, &CHttpJsonServer::m_parser_settings, m_buffer, m_request_len + content_len);

            LOGI("%s m_pos %d m_request_len %d  content_len %d parsed %d ",__FILE__, m_pos,m_request_len, content_len, parsed);

            if (parsed != (m_request_len + content_len)) {
                LOGI("%s m_pos %d m_request_len %d  content_len %d parsed %d http_parser_execute error",__FILE__, m_pos,m_request_len, content_len, parsed);
                parsed = m_request_len + content_len;
            }

            if(HPE_OK == HTTP_PARSER_ERRNO(&m_parser)){
                if (!http_should_keep_alive(&m_parser)) {
                    return close_connection();
                }else{
                    LOGI("releaseResource");
                    releaseResource();
                }
            }else{
                LOGE("%s http_parser_execute failed %d %s",__FILE__, HTTP_PARSER_ERRNO(&m_parser),http_errno_name(HTTP_PARSER_ERRNO(&m_parser)));
            }

            //shift processed request, discarding it
            memmove(m_buffer, m_buffer+parsed, m_pos - parsed);

            m_pos -= parsed;
            memset(m_buffer + m_pos, 0,m_buffer_size - m_pos - 1);

            goto done;
        }
    }

}

int CHttpConnection::start()
{
    do {
        ITransport::sock = m_fd;
        sprintf(this->token, "httpc");
        this->PollName = HTTPD_POLL_NAME;
        sp<SocketPoller> poller = SocketPoller::Instance(ITransport::PollName);
        BREAK_IF(poller == NULL);

        poller->AddTransport(this);

        http_parser_init(&m_parser, HTTP_BOTH);
    }while (0);

    return 0;
}


int CHttpConnection::sendReq(cJSON *root)
{
    //todo how to deal with response
    char * str_result = cJSON_Print(root);
    sprintf(http_resp,
            "POST /com/grandstream/httpjson/avs HTTP/1.1\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: keep-alive\r\n"
            "\r\n%s\n", strlen(str_result)+1, str_result);
    printf("\n [  %s  ]\n", http_resp);
    int nbytes = send(m_fd, http_resp, strlen(http_resp), MSG_NOSIGNAL);
    if (nbytes == -1) {
        LOGE("CHttpConnection sendReq error %d %s\n", errno, strerror(errno));
    }

    free(str_result);
    return 0;
}

int CHttpConnection::send_response(char *response)
{
    //int nbytes = write(m_fd, response, strlen(response));
    int nbytes = send(m_fd, response, strlen(response), MSG_NOSIGNAL);
    if (nbytes == -1) {
        LOGE("CHttpConnection send_response error %d %s\n", errno, strerror(errno));
    }

    return 0;
}
#if 0
int CHttpConnection::send_error(int code, char* message,cJSON * id)
{
    int return_value = 0;
    cJSON *result_root = cJSON_CreateObject();
    cJSON *error_root = cJSON_CreateObject();
    cJSON_AddNumberToObject(error_root, "code", code);
    cJSON_AddStringToObject(error_root, "message", message);
    cJSON_AddItemToObject(result_root, "error", error_root);
    if (id == NULL) {
        cJSON_AddNullToObject(result_root, "id");
    }else{
        cJSON_AddItemToObject(result_root, "id", id);
    }
    char * str_result = cJSON_Print(result_root);
    return_value = send_response(str_result);
    free(str_result);
    cJSON_Delete(result_root);
    free(message);
    return return_value;
}

int CHttpConnection::create_error(int code, char* message,cJSON * id,cJSON **result_root )
{
    int return_value = 0;
    *result_root = cJSON_CreateObject();
    cJSON *error_root = cJSON_CreateObject();
    cJSON_AddNumberToObject(error_root, "code", code);
    cJSON_AddStringToObject(error_root, "message", message);
    cJSON_AddItemToObject(*result_root, "error", error_root);
    if (id == NULL) {
        cJSON_AddNullToObject(*result_root, "id");
    }else{
        cJSON_AddItemToObject(*result_root, "id", id);
    }
    //char * str_result = cJSON_Print(result_root);
    //return_value = send_response(conn, str_result);
    //free(str_result);
    //cJSON_Delete(result_root);
    free(message);
    return return_value;
}

int CHttpConnection::send_result( cJSON * result,cJSON * id)
{
    int return_value = 0;
    cJSON *result_root = cJSON_CreateObject();
    //cJSON_AddStringToObject(result_root, "jsonrpc", "2.0");
    if (result)
        cJSON_AddItemToObject(result_root, "result", result);
    cJSON_AddItemToObject(result_root, "id", id);

    char * str_result = cJSON_Print(result_root);
    return_value = send_response(str_result);
    free(str_result);
    cJSON_Delete(result_root);
    return return_value;
}

int CHttpConnection::create_result(cJSON * result,cJSON * id, cJSON **result_root)
{
    int return_value = 0;
    *result_root = cJSON_CreateObject();
    //cJSON_AddStringToObject(*result_root, "jsonrpc", "2.0");
    if (result)
        cJSON_AddItemToObject(*result_root, "result", result);
    cJSON_AddItemToObject(*result_root, "id", id);

    //char * str_result = cJSON_Print(result_root);
    //return_value = send_response(conn, str_result);
    //free(str_result);
    //cJSON_Delete(result_root);
    return return_value;
}
#endif
void CHttpConnection::close_connection(struct ev_loop *loop, ev_io *w)
{
    ev_io_stop(loop, w);

    close(m_fd);
    free(m_buffer);

    CHttpJsonServer::Instance()->DelConnection(this);
}

void CHttpConnection::close_connection()
{
    do {
        sp<SocketPoller> poller = SocketPoller::Instance(ITransport::PollName);
        BREAK_IF(poller == NULL);

        poller->CloseTransport(this);
    }while (0);

    free(m_buffer);

    CHttpJsonServer::Instance()->DelConnection(this);
}

/*
 * Check whether full request is buffered. Return:
 *   -1         if request is malformed
 *    0         if request is not yet fully buffered
 *   >0         actual request length, including last \r\n\r\n
 */
int CHttpConnection::get_request_len(const char *buf, size_t buflen)
{
    const char  *s, *e;
    int     len = 0;

    for (s = buf, e = s + buflen - 1; len <= 0 && s < e; s++)
        /* Control characters are not allowed but >=128 is. */
        if (!isprint(* (unsigned char *) s) && *s != '\r' &&
            *s != '\n' && * (unsigned char *) s < 128)
            len = -1;
        else if (s[0] == '\n' && s[1] == '\n')
            len = (int) (s - buf) + 2;
        else if (s[0] == '\n' && &s[1] < e &&
            s[1] == '\r' && s[2] == '\n')
            len = (int) (s - buf) + 3;

    return (len);
}


/*
 * Skip the characters until one of the delimiters characters found.
 * 0-terminate resulting word. Skip the rest of the delimiters if any.
 * Advance pointer to buffer to the next word. Return found 0-terminated word.
 */
char * CHttpConnection::skip(char **buf, const char *delimiters)
{
    char    *p, *begin_word, *end_word, *end_delimiters;

    begin_word = *buf;
    end_word = begin_word + strcspn(begin_word, delimiters);
    end_delimiters = end_word + strspn(end_word, delimiters);

    for (p = end_word; p < end_delimiters; p++)
        *p = '\0';

    *buf = end_delimiters;

    return (begin_word);
}

/*
 * Parse HTTP headers from the given buffer, advance buffer to the point
 * where parsing stopped.
 */
void CHttpConnection::parse_http_content_len(char **buf)
{
    int i;
    char *_buffer = (char *)malloc(m_request_len);

    memcpy(_buffer, *buf, m_request_len);
    char *p=_buffer;

    for (i = 0; i < 64; i++) {
        char *pField = skip(&_buffer, ": ");
        char *pValue = skip(&_buffer, "\r\n");
        if (pField[0] == '\0')
            break;

        if (!strcmp(pField, "Content-Length") ) {
            content_len = atoi(pValue);
            break;
        }
    }

    free(p);

}

void CHttpConnection::Reply(char *szResp)
{

    if (!http_should_keep_alive(&m_parser)) {
        sprintf(http_resp,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Connection: close\r\n"
                "\r\n%s\n", strlen(szResp)+1, szResp);
    }else{
        sprintf(http_resp,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Connection: keep-alive\r\n"
                "\r\n%s\n", strlen(szResp)+1, szResp);
    }

    send_response(http_resp);
}

bool CHttpConnection::ConnInit(const char *pszServiceID)
{
    bool bRet = false;

    do {
        if (pszServiceID == NULL) {
            LOGE("conn:%p pszServiceID is NULL!",this ,pszServiceID);
            break;
        }

        if (strlen(pszServiceID) > (sizeof(mServiceID) - 1)) {
            LOGE("conn:%p strlen(pszServiceID):%d is too large (must less then %d)",this, strlen(pszServiceID), (sizeof(mServiceID) - 1));
            break;
        }

        if (strlen(mServiceID)) {
            LOGE("conn:%p serviceID:%s is already exist!",this, mServiceID);
            break;
        }

        strcpy(mServiceID,pszServiceID);

        bRet = true;
    }while (0); 

    return bRet;
}


#if 0
void CHttpConnection::connection_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    sp<CHttpJsonServer> server = CHttpJsonServer::Instance();
    size_t bytes_read = 0;

    //get our 'subclassed' event watcher
    sp<CHttpConnection> conn = (CHttpConnection *) w->data;

    int fd = conn->m_fd;

    if (conn->m_pos == (conn->m_buffer_size - 1)) {
        char * new_buffer = (char *)realloc(conn->m_buffer, conn->m_buffer_size *= 2);
        if (new_buffer == NULL) {
            perror("Memory error");
            return conn->close_connection(loop, w);
        }
        LOGI("realloc buffer_size:%d",conn->m_buffer_size);
        conn->m_buffer = new_buffer;
        memset(conn->m_buffer + conn->m_pos, 0, conn->m_buffer_size - conn->m_pos);
    }
    // can not fill the entire buffer, string must be NULL terminated
    int max_read_size = conn->m_buffer_size - conn->m_pos - 1;
    if ((bytes_read = read(fd, conn->m_buffer + conn->m_pos, max_read_size))
            == -1) {
        LOGI("%d %s ,so close connection",errno, strerror(errno));
        return conn->close_connection(loop, w);
    }
    if (!bytes_read) {
        // client closed the sending half of the connection

        LOGI("read return 0, so close connection");
        if (conn->m_pos) {
            size_t parsed =  http_parser_execute(&conn->m_parser, &CHttpJsonServer::m_parser_settings, conn->m_buffer, conn->m_pos);

            if(HPE_OK == HTTP_PARSER_ERRNO(&conn->m_parser)){
                conn->m_pos -= parsed;
                if (!http_should_keep_alive(&conn->m_parser)) {
                    return conn->close_connection(loop, w);
                }else{
                    conn->releaseResource();
                    return;
                }
            }else{
                LOGI("http_parser_execute failed %d %s",HTTP_PARSER_ERRNO(&conn->m_parser),http_errno_name(HTTP_PARSER_ERRNO(&conn->m_parser)));
            }
        }
        return conn->close_connection(loop, w);
    } else {
        cJSON *root;
        char *end_ptr = NULL;
        conn->m_pos += bytes_read;

        if (conn->m_request_len <= 0) {
            conn->m_request_len = conn->get_request_len(conn->m_buffer, (size_t)conn->m_pos);
            if (conn->m_request_len > 0) {
                LOGI("request_len:%d  bytes_read:%d buffer_size:%d ",conn->m_request_len,bytes_read,conn->m_buffer_size);
            }else{
                LOGI("request_len:%d bytes_read:%d buffer_size:%d",conn->m_request_len,bytes_read,conn->m_buffer_size);
            }
        }
        if (conn->m_pos >= conn->m_request_len && conn->m_request_len>0 && !conn->content_len) {
            conn->parse_http_content_len(&conn->m_buffer);
        }

        if(conn->m_request_len > 0 && conn->content_len && conn->m_pos >= (conn->m_request_len + conn->content_len)){

            size_t parsed =  http_parser_execute(&conn->m_parser, &CHttpJsonServer::m_parser_settings, conn->m_buffer, conn->m_pos);

            if(HPE_OK == HTTP_PARSER_ERRNO(&conn->m_parser)){
                conn->m_pos -= parsed;
                if (!http_should_keep_alive(&conn->m_parser)) {
                    return conn->close_connection(loop, w);
                }else{
                    conn->releaseResource();
                    return ;
                }
            }else{
                LOGI("http_parser_execute failed %d %s",HTTP_PARSER_ERRNO(&conn->m_parser),http_errno_name(HTTP_PARSER_ERRNO(&conn->m_parser)));
            }

            //shift processed request, discarding it
            memmove(conn->m_buffer, conn->m_buffer+parsed, conn->m_pos - parsed);

            conn->m_pos -= parsed;
            memset(conn->m_buffer + conn->m_pos, 0,conn->m_buffer_size - conn->m_pos - 1);
        }
    }
}
#endif


