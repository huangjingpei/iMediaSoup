#define MS_CLASS "RTC::TcpServer"
// #define MS_LOG_DEV

#include "RTC/HttpServer.hpp"
#include "Logger.hpp"
#include "RTC/PortManager.hpp"
#include <string>


namespace RTC
{
	/* Static. */

	static constexpr size_t MaxTcpConnectionsPerServer{ 10 };

	/* Instance methods. */

	HttpServer::HttpServer(Listener* listener, RTC::HttpConnection::Listener* connListener, std::string& ip, std::string &port)
	  : // This may throw.
	    ::TcpServer::TcpServer(PortManager::BindTcp(ip, port), 256), listener(listener),
	    connListener(connListener)
	{
		MS_TRACE();
	}

	HttpServer::~HttpServer()
	{
		MS_TRACE();

		PortManager::UnbindTcp(this->localIp, this->localPort);
	}

	void HttpServer::UserOnTcpConnectionAlloc(::TcpConnection** connection)
	{
		MS_TRACE();

		// Allocate a new RTC::TcpConnection for the TcpServer to handle it.
		*connection = new RTC::HttpConnection(this->connListener, 65536);
	}

	void HttpServer::UserOnNewTcpConnection(::TcpConnection* connection)
	{
		MS_TRACE();

		// Allow just MaxTcpConnectionsPerServer.
		if (GetNumConnections() > MaxTcpConnectionsPerServer)
			delete connection;
	}

	void HttpServer::UserOnTcpConnectionClosed(::TcpConnection* connection, bool isClosedByPeer)
	{
		MS_TRACE();

		this->listener->OnRtcTcpConnectionClosed(
		  this, static_cast<RTC::HttpConnection*>(connection), isClosedByPeer);
	}
} // namespace RTC
