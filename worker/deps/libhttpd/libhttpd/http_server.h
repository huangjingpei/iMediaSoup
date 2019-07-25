#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__
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
#include "ABase.h"
#include "http_parser.h"
#include <ev.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <sys/time.h>
#include <utils/List.h>
#include "Poller.h"
#include "Transport.h"

using namespace android;


#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))
#define UNKNOWN_CONTENT_LENGTH  ((uint64_t) ~0)


class CHttpConnection;

class IHttpMsgObserver : public RefBase {
 public:
  virtual void OnReceiveHttpMsg(sp<CHttpConnection> _conn,const char *pData, size_t len) = 0;
  virtual void OnReleaseResouce() = 0;
  virtual ~IHttpMsgObserver() {}
};

class CHttpJsonServer : public ITransport {
public:
    static inline sp<CHttpJsonServer> Instance()
    {
        if (_instance == NULL) {
            Mutex::Autolock _l(cLock);
            do {
                if (_instance != NULL)
                    break;

                sp<CHttpJsonServer> ptr = new CHttpJsonServer();
                if (ptr == NULL)
                    break;
                _instance = ptr;
            }while(0);
        }

        return _instance;
    };

    status_t http_server_create(sp<IHttpMsgObserver> _observer, int port_number);
    void http_server_run();
    status_t http_server_stop();

    static http_parser_settings m_parser_settings;

    status_t AddConnection(sp<CHttpConnection> _conn);
    status_t DelConnection(sp<CHttpConnection> _conn);

    void http_server_trigger_cb(sp<CHttpConnection> _conn, const char *pData, size_t len);

    status_t sendMsg(cJSON *msg);//todo
private:
    virtual ~CHttpJsonServer();

    
    static void accept_cb(struct ev_loop *loop, ev_io *w, int revents);

    status_t http_server_start();
    status_t http_server_init_with_ev_loop(int port_number);

    static void* get_in_addr(struct sockaddr *sa);


private:

    REFLECT_READ_HANDLER(CHttpJsonServer, mHandler, handleRead);

    virtual void handleClose(){}
    virtual void handleFree(){}
    virtual void handleWrite(){}
    virtual void resetSocket() {}

    int                         m_port_number;
    //struct ev_loop*             m_ev_loop;
    //ev_io                       m_listen_watcher_io;
    int                         debug_level;

    List< sp<CHttpConnection> > mConnectionList;

    sp<IHttpMsgObserver>        mObserver;

    int                         mHttpd_sock;
private:
    static Mutex cLock;
    static sp<CHttpJsonServer> _instance;

    DISALLOW_IMPLICIT_CONSTRUCTORS(CHttpJsonServer);
};

#endif
