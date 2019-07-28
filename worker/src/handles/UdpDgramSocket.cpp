/**
 * NOTE: This code cannot log to the Channel since this is the base code of the
 * Channel.
 */

#define MS_CLASS "UdpDgramSocket"
// #define MS_LOG_DEV

#include "handles/UdpDgramSocket.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()

/* Static methods for UV callbacks. */

inline static void onAlloc(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf)
{
	auto* socket = static_cast<UdpDgramSocket*>(handle->data);

	if (socket == nullptr)
		return;

	socket->OnUvReadAlloc(suggestedSize, buf);
}

//inline static void onRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
//{
//	auto* socket = static_cast<UdpDgramSocket*>(handle->data);
//
//	if (socket == nullptr)
//		return;
//
//	socket->OnUvRead(nread, buf);
//}

inline static void onRead(uv_udp_t* handle,
                               ssize_t nread,
                               const uv_buf_t* buf,
                               const struct sockaddr* addr,
                               unsigned flags) {
	auto* socket = static_cast<UdpDgramSocket*>(handle->data);

	if (socket == nullptr)
		return;

	socket->OnUvRead(nread, buf, addr);
}
//inline static void onWrite(uv_write_t* req, int status)
//{
//	auto* writeData = static_cast<UdpDgramSocket::UvWriteData*>(req->data);
//	auto* handle    = req->handle;
//	auto* socket    = static_cast<UdpDgramSocket*>(handle->data);
//
//	// Delete the UvWriteData struct (which includes the uv_req_t and the store char[]).
//	std::free(writeData);
//
//	if (socket == nullptr)
//		return;
//
//	// Just notify the UdpDgramSocket when error.
//	if (status != 0)
//		socket->OnUvWriteError(status);
//}

inline static void onWrite(uv_udp_send_t* req, int status)
{
		auto* writeData = static_cast<UdpDgramSocket::UvWriteData*>(req->data);
		auto* handle    = req->handle;
		auto* socket    = static_cast<UdpDgramSocket*>(handle->data);

		// Delete the UvWriteData struct (which includes the uv_req_t and the store char[]).
		std::free(writeData);

		if (socket == nullptr)
			return;

		// Just notify the UdpDgramSocket when error.
		if (status != 0)
			socket->OnUvWriteError(status);
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

inline static void onShutdown(uv_shutdown_t* req, int /*status*/)
{
	auto* handle = req->handle;

	delete req;

	// Now do close the handle.
	uv_close(reinterpret_cast<uv_handle_t*>(handle), static_cast<uv_close_cb>(onClose));
}

/* Instance methods. */

UdpDgramSocket::UdpDgramSocket(size_t bufferSize) : bufferSize(bufferSize)
{
	MS_TRACE_STD();

	int err;

	this->uvHandle       = new uv_udp_t;
	this->uvHandle->data = (void*)this;
    struct sockaddr_in addr;

    uv_ip4_addr("0.0.0.0", 65532, &addr);
	err = uv_udp_init(DepLibUV::GetLoop(), this->uvHandle);

	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR_STD("uv_pipe_init() failed: %s", uv_strerror(err));
	}

//	err = uv_pipe_open(this->uvHandle, fd);
	err = uv_udp_bind(this->uvHandle, (const struct sockaddr*) &addr, UV_UDP_REUSEADDR);


	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

		MS_THROW_ERROR_STD("uv_pipe_open() failed: %s", uv_strerror(err));
	}

	// Start reading.
//	err = uv_read_start(
//	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
//	  static_cast<uv_alloc_cb>(onAlloc),
//	  static_cast<uv_read_cb>(onRead));

	err = uv_udp_recv_start(this->uvHandle,
			static_cast<uv_alloc_cb>(onAlloc),
			static_cast<uv_udp_recv_cb>(onRead)
			);
	if (err != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

		MS_THROW_ERROR_STD("uv_read_start() failed: %s", uv_strerror(err));
	}

	// NOTE: Don't allocate the buffer here. Instead wait for the first uv_alloc_cb().
}

UdpDgramSocket::~UdpDgramSocket()
{
	MS_TRACE_STD();

	if (!this->closed)
		Close();

	delete[] this->buffer;
}

void UdpDgramSocket::Close()
{
	MS_TRACE_STD();

	if (this->closed)
		return;

	int err;

	this->closed = true;

	// Tell the UV handle that the UdpDgramSocket has been closed.
	this->uvHandle->data = nullptr;

	// Don't read more.
	err = uv_read_stop(reinterpret_cast<uv_stream_t*>(this->uvHandle));

	if (err != 0)
		MS_ABORT("uv_read_stop() failed: %s", uv_strerror(err));

	// If there is no error and the peer didn't close its pipe side then close gracefully.
	if (!this->hasError && !this->isClosedByPeer)
	{
		// Use uv_shutdown() so pending data to be written will be sent to the peer before closing.
		auto req  = new uv_shutdown_t;
		req->data = (void*)this;
		err       = uv_shutdown(
      req, reinterpret_cast<uv_stream_t*>(this->uvHandle), static_cast<uv_shutdown_cb>(onShutdown));

		if (err != 0)
			MS_ABORT("uv_shutdown() failed: %s", uv_strerror(err));
	}
	// Otherwise directly close the socket.
	else
	{
		uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
	}
}

void UdpDgramSocket::Write(const uint8_t* data, size_t len)
{
	if (this->closed)
		return;

	if (len == 0)
		return;

	// First try uv_try_write(). In case it can not directly send all the given data
	// then build a uv_req_t and use uv_write().

	uv_buf_t buffer = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(data)), len);
	//int written     = uv_try_write(reinterpret_cast<uv_stream_t*>(this->uvHandle), &buffer, 1);
	int written = uv_udp_try_send(this->uvHandle, &buffer, 1, &remotAddr);
	// All the data was written. Done.
	if (written == static_cast<int>(len))
	{
		return;
	}
	// Cannot write any data at first time. Use uv_write().
	else if (written == UV_EAGAIN || written == UV_ENOSYS)
	{
		// Set written to 0 so pendingLen can be properly calculated.
		written = 0;
	}
	// Error. Should not happen.
	else if (written < 0)
	{
		MS_ERROR_STD("uv_try_write() failed, closing the socket: %s", uv_strerror(written));

		Close();

		return;
	}

	size_t pendingLen = len - written;

	// Allocate a special UvWriteData struct pointer.
	auto* writeData = static_cast<UvWriteData*>(std::malloc(sizeof(UvWriteData) + pendingLen));

	std::memcpy(writeData->store, data + written, pendingLen);
	writeData->req.data = (void*)writeData;

	buffer = uv_buf_init(reinterpret_cast<char*>(writeData->store), pendingLen);

//	int err = uv_write(
//	  &writeData->req,
//	  reinterpret_cast<uv_stream_t*>(this->uvHandle),
//	  &buffer,
//	  1,
//	  static_cast<uv_write_cb>(onWrite));

	int err = uv_udp_send(&writeData->req,this->uvHandle, &buffer, 1, &remotAddr, onWrite);

	if (err != 0)
		MS_ABORT("uv_write() failed: %s", uv_strerror(err));
}

inline void UdpDgramSocket::OnUvReadAlloc(size_t /*suggestedSize*/, uv_buf_t* buf)
{
	MS_TRACE_STD();

	if (this->closed)
		return;

	// If this is the first call to onUvReadAlloc() then allocate the receiving buffer now.
	if (this->buffer == nullptr)
		this->buffer = new uint8_t[this->bufferSize];

	// Tell UV to write after the last data byte in the buffer.
	buf->base = reinterpret_cast<char*>(this->buffer + this->bufferDataLen);

	// Give UV all the remaining space in the buffer.
	if (this->bufferSize > this->bufferDataLen)
	{
		buf->len = this->bufferSize - this->bufferDataLen;
	}
	else
	{
		buf->len = 0;

		MS_ERROR_STD("no available space in the buffer");
	}
}

inline void UdpDgramSocket::OnUvRead(ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr)
{
	MS_TRACE_STD();
	if (this->closed)
		return;

	if (nread == 0)
		return;

	// Data received.
	if (nread > 0)
	{
		memcpy(&remotAddr, addr, sizeof(struct sockaddr));
		// Update the buffer data length.
		this->bufferDataLen += static_cast<size_t>(nread);

		// Notify the subclass.
		UserOnUdpDgramRead();
	}
	// Peer disconneted.
	else if (nread == UV_EOF || nread == UV_ECONNRESET)
	{
		this->isClosedByPeer = true;

		// Close local side of the pipe.
		Close();

		// Notify the subclass.
		UserOnUdpDgramSocketClosed(this->isClosedByPeer);
	}
	// Some error.
	else
	{
		MS_ERROR_STD("read error, closing the pipe: %s", uv_strerror(nread));

		this->hasError = true;

		// Close the socket.
		Close();

		// Notify the subclass.
		UserOnUdpDgramSocketClosed(this->isClosedByPeer);
	}
}

inline void UdpDgramSocket::OnUvWriteError(int error)
{
	MS_TRACE_STD();

	if (this->closed)
		return;

	if (error != UV_EPIPE && error != UV_ENOTCONN)
		this->hasError = true;

	MS_ERROR_STD("write error, closing the pipe: %s", uv_strerror(error));

	Close();

	// Notify the subclass.
	UserOnUdpDgramSocketClosed(this->isClosedByPeer);
}
