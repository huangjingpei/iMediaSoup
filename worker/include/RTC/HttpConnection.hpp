#ifndef MS_RTC_TCP_CONNECTION_HPP
#define MS_RTC_TCP_CONNECTION_HPP

#include "common.hpp"
#include "handles/TcpConnection.hpp"
#include "Channel/Request.hpp"

namespace RTC
{
	class HttpConnection : public ::TcpConnection
	{
	public:
		class Listener
		{
		public:
			virtual void OnTcpConnectionPacketReceived(
			  RTC::HttpConnection* connection, const Channel::Request* request) = 0;
		};

	public:
		HttpConnection(Listener* listener, size_t bufferSize);

	public:
		void Send(const uint8_t* data, size_t len);
		size_t GetRecvBytes() const;
		size_t GetSentBytes() const;

		/* Pure virtual methods inherited from ::TcpConnection. */
	public:
		void UserOnTcpConnectionRead() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		size_t frameStart{ 0 }; // Where the latest frame starts.
		size_t recvBytes{ 0 };
		size_t sentBytes{ 0 };
	};

	inline size_t HttpConnection::GetRecvBytes() const
	{
		return this->recvBytes;
	}

	inline size_t HttpConnection::GetSentBytes() const
	{
		return this->sentBytes;
	}


} // namespace RTC

#endif
