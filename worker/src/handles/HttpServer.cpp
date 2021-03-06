#include "../../include/handles/HttpServer.hpp"

#include <utility>

// 初始化HttpServer静态类成员
mg_serve_http_opts HttpServer::s_server_option;
std::string HttpServer::s_web_dir = "./web";
#ifdef WITH_UNIX_IPC
int HttpServer::m_unix_fd = -1;
char *HttpServer::m_unix_buf = NULL;
struct sockaddr_un HttpServer::s_unix_addr;
struct sockaddr_in HttpServer::addr;
#endif
std::unordered_map<std::string, ReqHandler> HttpServer::s_handler_map;
std::unordered_set<mg_connection *> HttpServer::s_websocket_session_set;

HttpServer::HttpServer() {
}

HttpServer::~HttpServer() {
#ifdef WITH_UNIX_IPC
	if (m_unix_buf != NULL) {
		free(m_unix_buf);
	}

	if (m_unix_fd != 0) {
		close(m_unix_fd);
	}
#endif
}

void HttpServer::Init(const std::string &port) {
	m_port = port;

	s_server_option.enable_directory_listing = "yes";
	s_server_option.document_root = s_web_dir.c_str();

#ifdef WITH_UNIX_IPC
	m_unix_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	bzero(&s_unix_addr, sizeof(s_unix_addr));
	s_unix_addr.sun_family = AF_UNIX;
	strcpy(s_unix_addr.sun_path, "/tmp/MSWorkerServer");
	m_unix_buf = (char *) malloc(20 * 1024);

	m_unix_fd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(65532);

//	struct timeval tv = { 0, 100000 };
//	setsockopt(m_unix_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
//	setsockopt(m_unix_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
	// 其他http设置

	// 开启 CORS，本项只针对主页加载有效
	// s_server_option.extra_headers = "Access-Control-Allow-Origin: *";
}
void HttpServer::unixSocketHandler(struct mg_connection *cgi_nc, int ev,
		void *ev_data) {
//	socklen_t len = 0;
//	int nbytes = -1;

	printf("en %d\n", ev);

	struct mg_connection *nc = (struct mg_connection *) cgi_nc->user_data;
	(void) ev_data;
	struct mbuf *io = &cgi_nc->recv_mbuf;
	printf("recv buf %s\n", io->buf);

	if (nc == NULL) {
		cgi_nc->flags |= MG_F_CLOSE_IMMEDIATELY;
		return;
	}
	switch (ev) {
	case MG_EV_POLL:
	case MG_EV_RECV:

		//获取worker进程的处理结果
//			len =sizeof(sizeof(s_unix_addr));
//			nbytes = recvfrom(m_unix_fd, m_unix_buf, 20*1024,
//				0, (struct sockaddr*) &s_unix_addr, &len);
//			if ((nbytes < 0) && (errno == EWOULDBLOCK)) {
//				nbytes = sprintf(m_unix_buf, "{ \"error\": \"/api/msv1 recv timeout.\"}");
//			}
		//把消息返回给客户端
		printf("recv buf %s\n", io->buf);
		SendHttpRsp(nc, std::string(io->buf, io->len));
//	case MG_EV_CLOSE:
//		nc->flags |= MG_F_SEND_AND_CLOSE;
//		break;
	}
}
bool HttpServer::Start() {
	mg_mgr_init(&m_mgr, NULL);
	mg_connection *connection = mg_bind(&m_mgr, m_port.c_str(),
			HttpServer::OnHttpWebsocketEvent);
	if (connection == NULL)
		return false;
#ifdef WITH_UNIX_IPC
//	mg_connection * unux_connection = mg_add_sock(&m_mgr, m_unix_fd, HttpServer::unixSocketHandler);
//	unux_connection->user_data = connection;
#endif
	// for both http and websocket
	mg_set_protocol_http_websocket(connection);

	printf("starting http server at port: %s\n", m_port.c_str());
	// loop
	while (true)
		mg_mgr_poll(&m_mgr, 500); // ms

	return true;
}

void HttpServer::OnHttpWebsocketEvent(mg_connection *connection, int event_type,
		void *event_data) {
	// 区分http和websocket
	if (event_type == MG_EV_HTTP_REQUEST) {
		http_message *http_req = (http_message *) event_data;
		HandleHttpEvent(connection, http_req);
	} else if (event_type == MG_EV_WEBSOCKET_HANDSHAKE_DONE
			|| event_type == MG_EV_WEBSOCKET_FRAME || event_type == MG_EV_CLOSE) {
		websocket_message *ws_message = (struct websocket_message *) event_data;
		HandleWebsocketMessage(connection, event_type, ws_message);
	}
}

// ---- simple http ---- //
static bool route_check(http_message *http_msg, const char *route_prefix) {
	if (mg_vcmp(&http_msg->uri, route_prefix) == 0)
		return true;
	else
		return false;

	// TODO: 还可以判断 GET, POST, PUT, DELTE等方法
	//mg_vcmp(&http_msg->method, "GET");
	//mg_vcmp(&http_msg->method, "POST");
	//mg_vcmp(&http_msg->method, "PUT");
	//mg_vcmp(&http_msg->method, "DELETE");
}

void HttpServer::AddHandler(const std::string &url, ReqHandler req_handler) {
	if (s_handler_map.find(url) != s_handler_map.end())
		return;

	s_handler_map.insert(std::make_pair(url, req_handler));
}

void HttpServer::RemoveHandler(const std::string &url) {
	auto it = s_handler_map.find(url);
	if (it != s_handler_map.end())
		s_handler_map.erase(it);
}

void HttpServer::SendHttpRsp(mg_connection *connection, std::string rsp) {
	// --- 未开启CORS
	// 必须先发送header, 暂时还不能用HTTP/2.0
	mg_printf(connection, "%s",
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	// 以json形式返回
	mg_printf_http_chunk(connection, "{ \"result\": %s }", rsp.c_str());
	// 发送空白字符快，结束当前响应
	mg_send_http_chunk(connection, "", 0);

	// --- 开启CORS
	/*mg_printf(connection, "HTTP/1.1 200 OK\r\n"
	 "Content-Type: text/plain\n"
	 "Cache-Control: no-cache\n"
	 "Content-Length: %d\n"
	 "Access-Control-Allow-Origin: *\n\n"
	 "%s\n", rsp.length(), rsp.c_str()); */
}

void HttpServer::HandleHttpEvent(mg_connection *connection,
		http_message *http_req) {
	std::string req_str = std::string(http_req->message.p,
			http_req->message.len);
	printf("got request: %s\n", req_str.c_str());

	// 先过滤是否已注册的函数回调
	std::string url = std::string(http_req->uri.p, http_req->uri.len);
	std::string body = std::string(http_req->body.p, http_req->body.len);
	auto it = s_handler_map.find(url);
	if (it != s_handler_map.end()) {
		ReqHandler handle_func = it->second;
		handle_func(url, body, connection, &HttpServer::SendHttpRsp);
	}

	// 其他请求
	if (route_check(http_req, "/")) // index page
		mg_serve_http(connection, http_req, s_server_option);
	else if (route_check(http_req, "/api/hello")) {
		// 直接回传
		SendHttpRsp(connection, "welcome to httpserver");
	} else if (route_check(http_req, "/api/msv1")) {
		//把内容转发到worker进程
		if (http_req->body.len == 0) {
//			SendHttpRsp(connection, "welcome to test restful interface");
			printf("request len %d\n", strlen("{\"id\":12345678,\"method\":\"worker.dump\",\"internal\":{},\"data:\":{}}"));
			int length = sprintf(m_unix_buf, "%zu:%s,",
					strlen("{\"id\":12345678,\"method\":\"worker.dump\",\"internal\":{},\"data:\":{}}"),
					"{\"id\":12345678,\"method\":\"worker.dump\",\"internal\":{},\"data:\":{}}");
			int nbytes = sendto(m_unix_fd, m_unix_buf, length, 0,
					(struct sockaddr*) &addr, sizeof(s_unix_addr));
			printf("nbytes %d length %d m_unix_buf:%s\n", nbytes, length, m_unix_buf);

			socklen_t len =sizeof(sizeof(s_unix_addr));
			nbytes = recvfrom(m_unix_fd, m_unix_buf, 20*1024,
				0, (struct sockaddr*) &addr, &len);
			if ((nbytes < 0) && (errno == EWOULDBLOCK)) {
				nbytes = sprintf(m_unix_buf, "{ \"error\": \"/api/msv1 recv timeout.\"}");
			}
			printf("recvbuf %s\n", m_unix_buf);
			SendHttpRsp(connection, std::string(m_unix_buf, nbytes));

		} else {
#ifdef WITH_UNIX_IPC
			//Format the send buf as "12:hello world!,";
			int length = sprintf(m_unix_buf, "%zu:%s,", http_req->body.len,
					http_req->body.p);
			int nbytes = sendto(m_unix_fd, m_unix_buf, length, 0,
					(struct sockaddr*) &s_unix_addr, sizeof(s_unix_addr));
			if ((nbytes < 0) && (errno == EWOULDBLOCK)) {
				nbytes = sprintf(m_unix_buf,
						"{ \"error\": \"/api/msv1 send timeout.\"}");
				SendHttpRsp(connection, std::string(m_unix_buf, nbytes));
			}
#else
			SendHttpRsp(connection, "welcome to test restful interface.");
#endif

		}
	} else if (route_check(http_req, "/api/sum")) {
		// 简单post请求，加法运算测试
		char n1[100], n2[100];
		double result;

		/* Get form variables */
		mg_get_http_var(&http_req->body, "n1", n1, sizeof(n1));
		mg_get_http_var(&http_req->body, "n2", n2, sizeof(n2));

		/* Compute the result and send it back as a JSON object */
		result = strtod(n1, NULL) + strtod(n2, NULL);
		SendHttpRsp(connection, std::to_string(result));
	} else {
		mg_printf(connection, "%s", "HTTP/1.1 501 Not Implemented\r\n"
				"Content-Length: 0\r\n\r\n");
	}
}

// ---- websocket ---- //
int HttpServer::isWebsocket(const mg_connection *connection) {
	return connection->flags & MG_F_IS_WEBSOCKET;
}

void HttpServer::HandleWebsocketMessage(mg_connection *connection,
		int event_type, websocket_message *ws_msg) {
	if (event_type == MG_EV_WEBSOCKET_HANDSHAKE_DONE) {
		printf("client websocket connected\n");
		// 获取连接客户端的IP和端口
		char addr[32];
		mg_sock_addr_to_str(&connection->sa, addr, sizeof(addr),
				MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
		printf("client addr: %s\n", addr);

		// 添加 session
		s_websocket_session_set.insert(connection);

		SendWebsocketMsg(connection, "client websocket connected");
	} else if (event_type == MG_EV_WEBSOCKET_FRAME) {
		mg_str received_msg = { (char *) ws_msg->data, ws_msg->size };

		char buff[1024] = { 0 };
		strncpy(buff, received_msg.p, received_msg.len); // must use strncpy, specifiy memory pointer and length

		// do sth to process request
		printf("received msg: %s\n", buff);
		SendWebsocketMsg(connection,
				"send your msg back: " + std::string(buff));
		//BroadcastWebsocketMsg("broadcast msg: " + std::string(buff));
	} else if (event_type == MG_EV_CLOSE) {
		if (isWebsocket(connection)) {
			printf("client websocket closed\n");
			// 移除session
			if (s_websocket_session_set.find(connection)
					!= s_websocket_session_set.end())
				s_websocket_session_set.erase(connection);
		}
	}
}

void HttpServer::SendWebsocketMsg(mg_connection *connection, std::string msg) {
	mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, msg.c_str(),
			strlen(msg.c_str()));
}

void HttpServer::BroadcastWebsocketMsg(std::string msg) {
	for (mg_connection *connection : s_websocket_session_set)
		mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, msg.c_str(),
				strlen(msg.c_str()));
}

bool HttpServer::Close() {
	mg_mgr_free(&m_mgr);
	return true;
}
#include "common.hpp"

//using namespace std;
//int main() {
//	std::string port = "7999";
//	auto http_server = std::shared_ptr < HttpServer > (new HttpServer);
//	http_server->Init(port);
//	http_server->Start();
//	while(1) {
//		sleep(10);
//	}
//	return 0;
//}

