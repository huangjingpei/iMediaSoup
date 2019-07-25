#define MS_CLASS "RTC::TcpConnection"
// #define MS_LOG_DEV

#include "RTC/HttpConnection.hpp"
#include "MediaSoupErrors.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove(), std::memcpy()
extern "C" {
#include <netstring.h>
}

namespace RTC
{
	/* Static. */

	static constexpr size_t ReadBufferSize{ 65536 };
	static uint8_t ReadBuffer[ReadBufferSize];

	/* Instance methods. */

	HttpConnection::HttpConnection(Listener* listener, size_t bufferSize)
	  : ::TcpConnection::TcpConnection(bufferSize), listener(listener)
	{
		MS_TRACE();
	}

	void HttpConnection::UserOnTcpConnectionRead()
	{
		MS_TRACE_STD();

		// Be ready to parse more than a single message in a single TCP chunk.
		while (true)
		{
			if (IsClosed())
				return;

			size_t readLen  = this->bufferDataLen - this->frameStart;
			char* jsonStart = nullptr;
			size_t jsonLen;
			int nsRet = netstring_read(
			  reinterpret_cast<char*>(this->buffer + this->frameStart), readLen, &jsonStart, &jsonLen);

			if (nsRet != 0)
			{
				switch (nsRet)
				{
					case NETSTRING_ERROR_TOO_SHORT:
					{
						// Check if the buffer is full.
						if (this->bufferDataLen == this->bufferSize)
						{
							// First case: the incomplete message does not begin at position 0 of
							// the buffer, so move the incomplete message to the position 0.
							if (this->frameStart != 0)
							{
								std::memmove(this->buffer, this->buffer + this->frameStart, readLen);
								this->frameStart      = 0;
								this->bufferDataLen = readLen;
							}
							// Second case: the incomplete message begins at position 0 of the buffer.
							// The message is too big, so discard it.
							else
							{
								MS_ERROR_STD(
								  "no more space in the buffer for the unfinished message being parsed, "
								  "discarding it");

								this->frameStart      = 0;
								this->bufferDataLen = 0;
							}
						}

						// Otherwise the buffer is not full, just wait.
						return;
					}

					case NETSTRING_ERROR_TOO_LONG:
					{
						MS_ERROR_STD("NETSTRING_ERROR_TOO_LONG");

						break;
					}

					case NETSTRING_ERROR_NO_COLON:
					{
						MS_ERROR_STD("NETSTRING_ERROR_NO_COLON");

						break;
					}

					case NETSTRING_ERROR_NO_COMMA:
					{
						MS_ERROR_STD("NETSTRING_ERROR_NO_COMMA");

						break;
					}

					case NETSTRING_ERROR_LEADING_ZERO:
					{
						MS_ERROR_STD("NETSTRING_ERROR_LEADING_ZERO");

						break;
					}

					case NETSTRING_ERROR_NO_LENGTH:
					{
						MS_ERROR_STD("NETSTRING_ERROR_NO_LENGTH");

						break;
					}
				}

				// Error, so reset and exit the parsing loop.
				this->frameStart      = 0;
				this->bufferDataLen = 0;

				return;
			}

			// If here it means that jsonStart points to the beginning of a JSON string
			// with jsonLen bytes length, so recalculate readLen.
			readLen =
			  reinterpret_cast<const uint8_t*>(jsonStart) - (this->buffer + this->frameStart) + jsonLen + 1;

			try
			{
				json jsonRequest = json::parse(jsonStart, jsonStart + jsonLen);

				Channel::Request* request{ nullptr };

				try
				{
//					/request = new Channel::Request(this, jsonRequest);
				}
				catch (const MediaSoupError& error)
				{
					MS_ERROR_STD("discarding wrong Channel request");
				}

				if (request != nullptr)
				{
					// Notify the listener.
					try
					{
						this->listener->OnTcpConnectionPacketReceived(this, request);
					}
					catch (const MediaSoupTypeError& error)
					{
						request->TypeError(error.what());
					}
					catch (const MediaSoupError& error)
					{
						request->Error(error.what());
					}

					// Delete the Request.
					delete request;
				}
			}
			catch (const json::parse_error& error)
			{
				MS_ERROR_STD("JSON parsing error: %s", error.what());
			}

			// If there is no more space available in the buffer and that is because
			// the latest parsed message filled it, then empty the full buffer.
			if ((this->frameStart + readLen) == this->bufferSize)
			{
				this->frameStart      = 0;
				this->bufferDataLen = 0;
			}
			// If there is still space in the buffer, set the beginning of the next
			// parsing to the next position after the parsed message.
			else
			{
				this->frameStart += readLen;
			}

			// If there is more data in the buffer after the parsed message
			// then parse again. Otherwise break here and wait for more data.
			if (this->bufferDataLen > this->frameStart)
			{
				continue;
			}

			break;
		}
	}

	void HttpConnection::Send(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Update sent bytes.
		this->sentBytes += len;

		// Write according to Framing RFC 4571.

		uint8_t frameLen[2];

		Utils::Byte::Set2Bytes(frameLen, 0, len);
		Write(frameLen, 2, data, len);
	}


} // namespace RTC
