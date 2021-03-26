#pragma once
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <dpp/sslclient.h>

/* Websocket connection status */
enum WSState {
	HTTP_HEADERS,	/* Sending/receiving HTTP headers prior to protocol switch */
	CONNECTED	/* Connected, upgraded and sending/receiving frames */
};

/* Low-level websocket opcodes for frames */
enum OpCode
{
        OP_CONTINUATION = 0x00,	/* Continuation */
        OP_TEXT = 0x01,		/* Text frame */
        OP_BINARY = 0x02,	/* Binary frame */
        OP_CLOSE = 0x08,	/* Close notification with close code */
        OP_PING = 0x09,		/* Low level ping */
        OP_PONG = 0x0a		/* Low level pong */
};

/* Implements a websocket client based on the SSL client */
class WSClient : public SSLClient
{
	/* Connection key used in the HTTP headers */
	std::string key;

	/* Current websocket state */
	WSState state;

	/* HTTP headers received on connecting/upgrading */
	std::map<std::string, std::string> HTTPHeaders;

	/* Parse headers for a websocket frame from the buffer */
	bool parseheader(std::string &buffer);

	/* Unpack a frame and pass completed frames up the stack */
	bool unpack(std::string &buffer, uint32_t offset, bool first = true);

	/* Fill a header for outbound messages */
	size_t FillHeader(unsigned char* outbuf, size_t sendlength, OpCode opcode);
public:

	/* Connect to a specific websocket server */
        WSClient(const std::string &hostname, const std::string &port = "443");

	/* Destructor */
        virtual ~WSClient();

	/* Write to websocket. Encapsulates data in frames if the status is CONNECTED */
        virtual void write(const std::string &data);

	/* Processes incoming frames from the SSL socket input buffer */
        virtual bool HandleBuffer(std::string &buffer);

	/* Close websocket */
        virtual void close();

	/* Receives raw frame content only without headers */
	virtual bool HandleFrame(const std::string &buffer);

	/* Called upon error frame */
	virtual void Error(uint32_t errorcode);
};

