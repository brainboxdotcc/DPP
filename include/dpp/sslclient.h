#pragma once
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* Implements a simple SSL stream client */
class SSLClient
{
protected:
	/* Input buffer received from openssl */
	std::string buffer;

	/* Output buffer for sending to openssl */
	std::string obuffer;

	/* True if in nonblocking mode. The socket switches to nonblocking mode
	 * once ReadLoop is called.
	 */
	bool nonblocking;

	/* Raw file descriptor */
	int sfd;

	/* SSL session */
	SSL* ssl;

	/* SSL context */
	SSL_CTX* ctx;

	/* SSL cipher in use */
	std::string cipher;

	/* For timers */
	time_t last_tick;

	std::string hostname;

	std::string port;

	virtual void OneSecondTimer();

	virtual void Connect();
public:
	/* Connect to a specified host and port */
	SSLClient(const std::string &_hostname, const std::string &_port = "443");

	/* Nonblocking I/O loop */
	void ReadLoop();

	/* Destructor */
	virtual ~SSLClient();

	/* Handle input from the input buffer */
	virtual bool HandleBuffer(std::string &buffer);

	/* Write to the output buffer */
	virtual void write(const std::string &data);

	/* Close SSL connection */
	virtual void close();
};

