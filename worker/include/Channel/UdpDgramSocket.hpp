#ifndef MS_CHANNEL_UDP_DGRAM_SOCKET_HPP
#define MS_CHANNEL_UDP_DGRAM_SOCKET_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "handles/UdpDgramSocket.hpp"

namespace Channel
{
	class UdpDgramSocket : public ::UdpDgramSocket
	{
	public:
		class Listener
		{
		public:
			virtual void OnChannelRequest(Channel::UdpDgramSocket* channel, Channel::Request* request) = 0;
			virtual void OnChannelRemotelyClosed(Channel::UdpDgramSocket* channel) = 0;
		};

	public:
		explicit UdpDgramSocket();

	public:
		void SetListener(Listener* listener);
		void Send(json& jsonMessage);
		void SendLog(char* nsPayload, size_t nsPayloadLen);
		void SendBinary(const uint8_t* nsPayload, size_t nsPayloadLen);

		/* Pure virtual methods inherited from ::UdpDgramSocket. */
	public:
		void UserOnUdpDgramRead() override;
		void UserOnUdpDgramSocketClosed(bool isClosedByPeer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		size_t msgStart{ 0 }; // Where the latest message starts.
	};
} // namespace Channel

#endif
