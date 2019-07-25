#define MS_CLASS "httpd"
#include <stdio.h>
#include <unistd.h>
#include <RTC/HttpServer.hpp>
#include "DepLibUV.hpp"
#include "MediaSoupErrors.hpp"
#define MS_LOG_DEV 1

class JsonHttpServer : public RTC::HttpServer::Listener ,
					    public RTC::HttpConnection::Listener {
public:
	JsonHttpServer();
	virtual ~JsonHttpServer();
	void OnRtcTcpConnectionClosed(
	  RTC::HttpServer* tcpServer, RTC::HttpConnection* connection, bool isClosedByPeer) override;
	void OnTcpConnectionPacketReceived(
			RTC::HttpConnection* connection, const Channel::Request* request) override;


};

JsonHttpServer::JsonHttpServer() {

}

JsonHttpServer::~JsonHttpServer() {

}

void JsonHttpServer::OnRtcTcpConnectionClosed(
	  RTC::HttpServer* tcpServer, RTC::HttpConnection* connection, bool isClosedByPeer) {
	printf("OnRtcTcpConnectionClosed\n");
}

void JsonHttpServer::OnTcpConnectionPacketReceived(
		RTC::HttpConnection* connection, const Channel::Request* request) {
	printf("OnTcpConnectionPacketReceived\n");
}


int main() {
	DepLibUV::ClassInit();

	JsonHttpServer *jsonHttpServer = new JsonHttpServer();
	std::string host = "127.0.0.1";
	std::string port = "6000";
	RTC::HttpServer *server = new RTC::HttpServer(jsonHttpServer, jsonHttpServer, host, port);
	printf("server %p\n", server);

	while (1) {
		DepLibUV::RunLoop();

	}
	return 0;
}
