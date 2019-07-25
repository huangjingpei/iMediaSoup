#include "http_server.h"
#include "http_connection.h"
#include "http_request_cb.h"
#include "utility.h"

Mutex CHttpJsonServer::cLock;
sp<CHttpJsonServer> CHttpJsonServer::_instance;
http_parser_settings CHttpJsonServer::m_parser_settings;



CHttpJsonServer::CHttpJsonServer():
                mHandler(new NetHandler(this)),
                m_port_number(0),
                //m_ev_loop(NULL),
                debug_level(0),
                mHttpd_sock(-1)
{
}

CHttpJsonServer::~CHttpJsonServer()
{
    if (mHttpd_sock > 0) {
        close(mHttpd_sock);
    }
}

// get sockaddr, IPv4 or IPv6:
void *CHttpJsonServer::get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void CHttpJsonServer::accept_cb(struct ev_loop *loop, ev_io *w, int revents) 
{
    char s[INET6_ADDRSTRLEN];
    sp<CHttpConnection>  connection_watcher;
    CHttpConnection::create(connection_watcher);
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    sin_size = sizeof their_addr;

    int fd = accept(w->fd, (struct sockaddr *) &their_addr,&sin_size);
    if (fd == -1) {
        perror("accept");
        connection_watcher = NULL;
    } else {
        if (((CHttpJsonServer *) w->data)->debug_level) {
            inet_ntop(their_addr.ss_family,
                    CHttpJsonServer::get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
            //printf("server: got connection from %s\n", s);
        }

        connection_watcher->init(fd, ((CHttpJsonServer *) w->data)->debug_level);
        connection_watcher->start(loop);

        ((CHttpJsonServer *) w->data)->AddConnection(connection_watcher);
    }
}

status_t CHttpJsonServer::AddConnection(sp<CHttpConnection> _conn)
{
    status_t err = UNKNOWN_ERROR;

    do {
        mConnectionList.push_back(_conn);
        err = NO_ERROR;
    }while (0);

    return err;
}

status_t CHttpJsonServer::DelConnection(sp<CHttpConnection> _conn)
{
    status_t err = UNKNOWN_ERROR;

    do {
        List< sp<CHttpConnection> >::iterator it;
        for (it = mConnectionList.begin();
             it != mConnectionList.end(); ++it)
        {
            if (_conn.get() ==  (*it).get()) {
                mConnectionList.erase(it);
                if (mObserver != NULL) {
                    mObserver->OnReleaseResouce();
                }
                break;
            }
        }
        err = NO_ERROR;
    }while (0);

    return err;
}


/// Reflected method
void CHttpJsonServer::handleRead()
{
    char s[INET6_ADDRSTRLEN];
    sp<CHttpConnection>  connection_watcher;
    CHttpConnection::create(connection_watcher);
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    sin_size = sizeof their_addr;

    int fd = accept(mHttpd_sock, (struct sockaddr *) &their_addr,&sin_size);
    if (fd == -1) {
        perror("accept");
        connection_watcher = NULL;
    } else {
        if (debug_level) {
            inet_ntop(their_addr.ss_family,
                    CHttpJsonServer::get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
            //printf("server: got connection from %s\n", s);
        }

        connection_watcher->init(fd, debug_level);
        connection_watcher->start();

        AddConnection(connection_watcher);
    }
}

status_t CHttpJsonServer::http_server_start() 
{
    status_t err = UNKNOWN_ERROR;

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in sockaddr;
    unsigned int len;
    int yes = 1;
    int rv;
    char PORT[6];
    sprintf(PORT, "%d", m_port_number);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    do {
        if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            break;
        }

        // loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((mHttpd_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
                    == -1) {
                perror("server: socket");
                continue;
            }

            if (setsockopt(mHttpd_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
                    == -1) {
                perror("setsockopt");
                break;
            }

            if (bind(mHttpd_sock, p->ai_addr, p->ai_addrlen) == -1) {
                close(mHttpd_sock);
                perror("server: bind");
                continue;
            }

            len = sizeof(sockaddr);
            if (getsockname(mHttpd_sock, (struct sockaddr *) &sockaddr, &len) == -1) {
                close(mHttpd_sock);
                perror("server: getsockname");
                continue;
            }
            m_port_number = ntohs( sockaddr.sin_port );
            printf("m_port_number:%d\n",m_port_number);
            break;
        }

        if (p == NULL) {
            fprintf(stderr, "server: failed to bind\n");
            break;
        }

        freeaddrinfo(servinfo); // all done with this structure

        if (listen(mHttpd_sock, 5) == -1) {
            perror("listen");
            break;
        }
        if (debug_level){
            printf("server: waiting for connections...\n");
        }

        //ev_io_init(&m_listen_watcher_io, accept_cb, sockfd, EV_READ);
        //m_listen_watcher_io.data = this;
        //ev_io_start(m_ev_loop, &m_listen_watcher_io);

        ITransport::sock = mHttpd_sock;
        sprintf(this->token, "httpd");
        this->PollName = HTTPD_POLL_NAME;
        sp<SocketPoller> poller = SocketPoller::Instance(ITransport::PollName);
        BREAK_IF(poller == NULL);

        poller->AddTransport(this);

        err = NO_ERROR;
    }while (0);

    return err;
}


status_t CHttpJsonServer::http_server_init_with_ev_loop(int port_number) 
{
    status_t err = UNKNOWN_ERROR;

    m_port_number = port_number;
    char * debug_level_env = getenv("JRPC_DEBUG");
    if (debug_level_env == NULL)
        debug_level = 255;
    else {
        debug_level = strtol(debug_level_env, NULL, 10);
        printf("JSONRPC-C Debug level %d\n", debug_level);
    }
    err = http_server_start();

    return err;
}

status_t CHttpJsonServer::http_server_create(sp<IHttpMsgObserver> _observer, int port_number) 
{
    status_t err = UNKNOWN_ERROR;

    //m_ev_loop = EV_DEFAULT;

    mObserver = _observer;

    CHttpJsonServer::m_parser_settings.on_url=http_request_url_cb;
    CHttpJsonServer::m_parser_settings.on_header_field=http_header_field_cb;
    CHttpJsonServer::m_parser_settings.on_header_value=http_header_value_cb;
    CHttpJsonServer::m_parser_settings.on_headers_complete=http_headers_complete_cb;
    CHttpJsonServer::m_parser_settings.on_body=http_body_cb;
    CHttpJsonServer::m_parser_settings.on_message_begin=http_message_begin_cb;
    CHttpJsonServer::m_parser_settings.on_message_complete=http_message_complete_cb;
    CHttpJsonServer::m_parser_settings.on_status=http_response_status_cb;

    err = http_server_init_with_ev_loop( port_number);

    return err;
}


void CHttpJsonServer::http_server_run()
{
    //ev_loop(m_ev_loop, 0);
}

status_t CHttpJsonServer::http_server_stop() 
{
    //ev_unloop(m_ev_loop, EVBREAK_ALL);
    return NO_ERROR;
}

void CHttpJsonServer::http_server_trigger_cb(sp<CHttpConnection> _conn,const char *pData, size_t len)
{
    if (mObserver != NULL) {
        mObserver->OnReceiveHttpMsg(_conn,pData,len);
    }
}

status_t CHttpJsonServer::sendMsg(cJSON *msg)
{
    for (const auto & _it : mConnectionList) {
        _it->sendReq(msg);
    }
}
