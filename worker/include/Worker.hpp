#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "Channel/UdpDgramSocket.hpp"
#include "RTC/Router.hpp"
#include "handles/SignalsHandler.hpp"
#include <string>
#include <unordered_map>

using json = nlohmann::json;

class Worker : public Channel::UdpDgramSocket::Listener, public SignalsHandler::Listener
{
public:
	explicit Worker(Channel::UdpDgramSocket* channel);
	~Worker();

private:
	void Close();
	void FillJson(json& jsonObject) const;
	void SetNewRouterIdFromRequest(Channel::Request* request, std::string& routerId) const;
	RTC::Router* GetRouterFromRequest(Channel::Request* request) const;

	/* Methods inherited from Channel::UdpDgramSocket::Listener. */
public:
	void OnChannelRequest(Channel::UdpDgramSocket* channel, Channel::Request* request) override;
	void OnChannelRemotelyClosed(Channel::UdpDgramSocket* channel) override;

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

private:
	// Passed by argument.
	Channel::UdpDgramSocket* channel{ nullptr };
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	// Others.
	bool closed{ false };
	std::unordered_map<std::string, RTC::Router*> mapRouters;
};

#endif
