#ifndef MS_UNIX_STREAM_SOCKET_HPP
#define MS_UNIX_STREAM_SOCKET_HPP

#include "common.hpp"
#include <uv.h>
#include <string>

class UdpDgramSocket
{
public:
	/* Struct for the data field of uv_req_t when writing data. */
	struct UvWriteData
	{
		uv_udp_send_t req;
		uint8_t store[1];
	};

public:
	UdpDgramSocket(size_t bufferSize);
	UdpDgramSocket& operator=(const UdpDgramSocket&) = delete;
	UdpDgramSocket(const UdpDgramSocket&)            = delete;
	virtual ~UdpDgramSocket();

public:
	void Close();
	bool IsClosed() const;
	void Write(const uint8_t* data, size_t len);
	void Write(const std::string& data);

	/* Callbacks fired by UV events. */
public:
	void OnUvReadAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRead(ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr);
	void OnUvWriteError(int error);

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnUdpDgramRead()                            = 0;
	virtual void UserOnUdpDgramSocketClosed(bool isClosedByPeer) = 0;

private:
	// Allocated by this.
	uv_udp_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	bool isClosedByPeer{ false };
	bool hasError{ false };
	struct sockaddr remotAddr;
protected:
	// Passed by argument.
	size_t bufferSize{ 0 };
	// Allocated by this.
	uint8_t* buffer{ nullptr };
	// Others.
	size_t bufferDataLen{ 0 };
};

/* Inline methods. */

inline bool UdpDgramSocket::IsClosed() const
{
	return this->closed;
}

inline void UdpDgramSocket::Write(const std::string& data)
{
	Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
}

#endif
