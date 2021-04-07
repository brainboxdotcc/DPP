#pragma once
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

/** Implements a simple non-blocking SSL stream client */
class SSLClient
{
protected:
	/** Input buffer received from openssl */
	std::string buffer;

	/** Output buffer for sending to openssl */
	std::string obuffer;

	/** True if in nonblocking mode. The socket switches to nonblocking mode
	 * once ReadLoop is called.
	 */
	bool nonblocking;

	/** Raw file descriptor of connection */
	int sfd;

	/** OpenSSL session */
	SSL* ssl;

	/** OpenSSL context */
	SSL_CTX* ctx;

	/** SSL cipher in use */
	std::string cipher;

	/** For timers */
	time_t last_tick;

	/** Hostname connected to */
	std::string hostname;

	/** Port connected to */
	std::string port;

	/** Bytes out */
	uint64_t bytes_out;

	/** Bytes in */
	uint64_t bytes_in;

	/** Called every second */
	virtual void OneSecondTimer();

	/** Start connection */
	virtual void Connect();
public:
	uint64_t GetBytesOut();
	uint64_t GetBytesIn();

	/** Connect to a specified host and port. Throws std::runtime_error on fatal error.
	 * @param _hostname The hostname to connect to
	 * @param _post the Port number to connect to
	 */
	SSLClient(const std::string &_hostname, const std::string &_port = "443");

	/** Nonblocking I/O loop */
	void ReadLoop();

	/** Destructor */
	virtual ~SSLClient();

	/** Handle input from the input buffer.
	 * @param buffer the buffer content. Will be modified removing any processed front elements
	 */
	virtual bool HandleBuffer(std::string &buffer);

	/** Write to the output buffer.
	 * @param data Data to be written to the buffer
	 */
	virtual void write(const std::string &data);

	/** Close SSL connection */
	virtual void close();
};

