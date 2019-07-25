#ifndef MS_RTC_TCP_SERVER_HPP
#define MS_RTC_TCP_SERVER_HPP

#include "common.hpp"
#include "RTC/HttpConnection.hpp"
#include "handles/TcpConnection.hpp"
#include "handles/TcpServer.hpp"
#include <string>
extern "C" {
	#include "http_parser.h"
}
namespace RTC
{
	class HttpServer : public ::TcpServer
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtcTcpConnectionClosed(
			  RTC::HttpServer* tcpServer, RTC::HttpConnection* connection, bool isClosedByPeer) = 0;
		};

	public:
		HttpServer(Listener* listener, RTC::HttpConnection::Listener* connListener, std::string& ip, std::string& port);
		~HttpServer() override;

		/* Pure virtual methods inherited from ::TcpServer. */
	public:
		void UserOnTcpConnectionAlloc(::TcpConnection** connection) override;
		void UserOnNewTcpConnection(::TcpConnection* connection) override;
		void UserOnTcpConnectionClosed(::TcpConnection* connection, bool isClosedByPeer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		RTC::HttpConnection::Listener* connListener{ nullptr };

	private:
		http_parser_settings m_parser_settings;
	};
} // namespace RTC

#endif
