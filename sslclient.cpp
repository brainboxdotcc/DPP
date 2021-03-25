#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <exception>
#include <string>
#include <iostream>
#include "sslclient.h"

const int ERROR_STATUS = -1;

SSLClient::SSLClient(const std::string &hostname, const std::string &port)
{
	const SSL_METHOD *method = TLS_client_method(); /* Create new client-method instance */
	ctx = SSL_CTX_new(method);
	if (ctx == nullptr)
		throw std::runtime_error("Failed to create SSL client context!");

	ssl = SSL_new(ctx);
	if (ssl == nullptr)
		throw std::runtime_error("SSL_new failed!");

	struct hostent *host;
	if ((host = gethostbyname(hostname.c_str())) == nullptr)
		throw std::runtime_error("Couldn't resolve hostname");

	struct addrinfo hints = {0}, *addrs;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
	if (status != 0)
		throw std::runtime_error(gai_strerror(status));

	int err;
	for (struct addrinfo *addr = addrs; addr != nullptr; addr = addr->ai_next) {
		sfd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
		if (sfd == ERROR_STATUS) {
			err = errno;
			continue;
		} else if (connect(sfd, addr->ai_addr, addr->ai_addrlen) == 0) {
			break;
		}
		err = errno;
		sfd = ERROR_STATUS;
		::close(sfd);
	}
	freeaddrinfo(addrs);

	if (sfd == ERROR_STATUS)
		throw std::runtime_error(strerror(err));

	SSL_set_fd(ssl, sfd);

	status = SSL_connect(ssl);
	if (status != 1) {
		throw std::runtime_error("SSL_connect error");
				//fprintf(stderr, "SSL_connect failed with SSL_get_error code %d\n", status);
	}

		printf("Connected with %s encryption\n", SSL_get_cipher(ssl));

}

void SSLClient::write(const std::string &data)
{
	if (ctx && ssl && sfd) {
		SSL_write(ssl, data.data(), data.length());
	}
}

void SSLClient::ReadLoop()
{
	const int readSize = 1024 * 1024;
	int received, count = 0;
	int TotalReceived = 0;
	fd_set fds;
	struct timeval timeout;
	char rawbuf[1024 * 1024];

	if (ctx && ssl && sfd)
	{
		while (true)
		{
			received = SSL_read(ssl, rawbuf, readSize);
			if (received > 0)
			{
				TotalReceived += received;
				buffer.append(rawbuf, received);
				if (!this->HandleBuffer(buffer)) {
					break;
				}
			}
			else
			{
				count++;
				int err = SSL_get_error(ssl, received);
				switch (err)
				{
					case SSL_ERROR_NONE:
					{
						// no real error, just try again...
						continue;
					}   

					case SSL_ERROR_ZERO_RETURN: 
					{
						// peer disconnected...
						break;
					}   

					case SSL_ERROR_WANT_READ: 
					{
						// no data available right now, wait a few seconds in case new data arrives...
						FD_ZERO(&fds);
						FD_SET(sfd, &fds);
						timeout.tv_sec = 5;
						timeout.tv_usec = 0;
						err = select(sfd + 1, &fds, NULL, NULL, &timeout);

						if (err > 0)
							continue; // more data to read...

						if (err == 0) {
							// timeout...
						} else {
							// error...
						}

						break;
					}

					case SSL_ERROR_WANT_WRITE: 
					{
						// socket not writable right now, wait a few seconds and try again...
						FD_ZERO(&fds);
						FD_SET(sfd, &fds);
						timeout.tv_sec = 5;
						timeout.tv_usec = 0;
						err = select(sfd + 1, NULL, &fds, NULL, &timeout);

						if (err > 0)
							continue; // can write more data now...
						if (err == 0) {
							// timeout...
						} else {
							// error...
						}

						break;
					}

					default:
					{
						break;
					}
				}
				break;
			}
		}
	}
}

bool SSLClient::HandleBuffer(std::string &buffer)
{
	return true;
}

void SSLClient::close()
{
	if (ctx && ssl && sfd) {
		SSL_free(ssl);
		::close(sfd);
		SSL_CTX_free(ctx);
		ctx = nullptr;
		ssl = nullptr;
		sfd = 0;
	}
}

SSLClient::~SSLClient()
{
	this->close();
}


