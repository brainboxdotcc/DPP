#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <exception>
#include <string>
#include <iostream>
#include <dpp/sslclient.h>

#define BUFSIZZ 10240
const int ERROR_STATUS = -1;

SSLClient::SSLClient(const std::string &hostname, const std::string &port)
{
	nonblocking = false;
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
	if (nonblocking) {
		obuffer += data;
	} else {
		SSL_write(ssl, data.data(), data.length());
	}
}

void SSLClient::ReadLoop()
{
	int width;
	int r,c2sl=0,c2s_offset=0;
	int read_blocked_on_write=0,write_blocked_on_read=0,read_blocked=0;
	fd_set readfds,writefds;
	int shutdown_wait=0;
	char c2s[BUFSIZZ],s2c[BUFSIZZ];
	int ofcmode;
		
	/*First we make the socket nonblocking*/
	ofcmode = fcntl(sfd,F_GETFL,0);
	ofcmode |= O_NDELAY;
	if (fcntl(sfd, F_SETFL, ofcmode)) {
		std::cout <<"Couldn't make socket nonblocking\n";
		return;
	}

	nonblocking = true;
	width=sfd+1;
		
	while(1) {
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		FD_SET(sfd,&readfds);

		/* If we're waiting for a read on the socket don't try to write to the server */
		if (!write_blocked_on_read) {
			if (c2sl || read_blocked_on_write) {
				FD_SET(sfd,&writefds);
			}
		}
			
		timeval ts;
		ts.tv_sec = 0;
		ts.tv_usec = 1000;
		r = select(width,&readfds,&writefds,0,&ts);
		if (r == 0)
			continue;

		/* Now check if there's data to read */
		if((FD_ISSET(sfd,&readfds) && !write_blocked_on_read) || (read_blocked_on_write && FD_ISSET(sfd,&writefds))) {
			do {
				read_blocked_on_write=0;
				read_blocked=0;
					
				r = SSL_read(ssl,s2c,BUFSIZZ);
				
				int e = SSL_get_error(ssl,r);

				switch(e){
					case SSL_ERROR_NONE:
						std::cout << "buffer append\n";
						buffer.append(s2c, r);
						this->HandleBuffer(buffer);
					break;
					case SSL_ERROR_ZERO_RETURN:
						/* End of data */
						if(!shutdown_wait)
							SSL_shutdown(ssl);
						return;
					break;
					case SSL_ERROR_WANT_READ:
						read_blocked=1;
					break;
							
					/* We get a WANT_WRITE if we're trying to rehandshake and we block on a write during that rehandshake.
					 * We need to wait on the socket to be writeable but reinitiate the read when it is
					 */
					case SSL_ERROR_WANT_WRITE:
						read_blocked_on_write=1;
					break;
					default:
						std::cout << "SSL read problem " << e << "\n";
						return;
					break;
				}

				/* We need a check for read_blocked here because SSL_pending() doesn't work properly during the
				 * handshake. This check prevents a busy-wait loop around SSL_read()
				 */
			} while (SSL_pending(ssl) && !read_blocked);
		}
			
		/* Check for input on the sendq */
		if (obuffer.length() && c2sl == 0) {
			memcpy(&c2s, obuffer.data(), obuffer.length() > BUFSIZZ ? BUFSIZZ : obuffer.length());
			c2sl = obuffer.length() > BUFSIZZ ? BUFSIZZ : obuffer.length();
			obuffer = obuffer.substr(c2sl, obuffer.length());
			std::cout << "New obuffer '" << obuffer << "' len " << obuffer.length() << "\n";
			c2s_offset = 0;
		}

		/* If the socket is writeable... */
		if ((FD_ISSET(sfd,&writefds) && c2sl) || (write_blocked_on_read && FD_ISSET(sfd,&readfds))) {
			write_blocked_on_read=0;
			/* Try to write */
			r = SSL_write(ssl, c2s + c2s_offset, c2sl);
			for (int v = c2s_offset; v < c2s_offset + c2sl; ++v) {
				std::cout << std::hex << ((uint16_t)c2s[v] & 0xff) << std::dec << " ";
			}
			std::cout << " -- written\n";
			
			switch(SSL_get_error(ssl,r)){
				/* We wrote something*/
				case SSL_ERROR_NONE:
					c2sl -= r;
					c2s_offset += r;
					std::cout << "wrote " << r << " offset now " << c2s_offset << " len now " << c2sl << "\n";
				break;
					
				/* We would have blocked */
				case SSL_ERROR_WANT_WRITE:
				break;
		
				/* We get a WANT_READ if we're trying to rehandshake and we block onwrite during the current connection.
				 * We need to wait on the socket to be readable but reinitiate our write when it is
				*/
				case SSL_ERROR_WANT_READ:
					write_blocked_on_read=1;
				break;
						
				/* Some other error */
				default:				
					std::cout << "SSL write problem";
				return;
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


