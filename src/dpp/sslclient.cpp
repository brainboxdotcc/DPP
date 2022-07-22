/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors 
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include <errno.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <io.h>
#pragma comment(lib,"ws2_32")
#else
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

#ifdef OPENSSL_SYS_WIN32
#undef X509_NAME
#undef X509_EXTENSIONS
#undef X509_CERT_PAIR
#undef PKCS7_ISSUER_AND_SERIAL
#undef OCSP_REQUEST
#undef OCSP_RESPONSE
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <exception>
#include <string>
#include <iostream>
#include <unordered_map>
#include <dpp/sslclient.h>
#include <dpp/exception.h>
#include <dpp/utility.h>
#include <dpp/dns.h>

/* Maximum allowed time in milliseconds for socket read/write timeouts and connect() */
#define SOCKET_OP_TIMEOUT 5000

namespace dpp {

/**
 * @brief This is an opaque class containing openssl library specific structures.
 * We define it this way so that the public facing D++ library doesn't require
 * the openssl headers be available to build against it.
 */
class openssl_connection {
public:
	/**
	 * @brief OpenSSL session
	 */
	SSL* ssl;
};

/**
 * @brief Keepalive cache record
 */
struct keepalive_cache_t {
	time_t created;
	openssl_connection* ssl;
	dpp::socket sfd;
};

/**
 * @brief OpenSSL context
 */
thread_local SSL_CTX* openssl_context = nullptr;

/**
 * @brief Keepalive sessions, per-thread
 */
thread_local std::unordered_map<std::string, keepalive_cache_t> keepalives;

/* NOTE: Upper bounds check not required: https://docs.microsoft.com/en-us/windows/win32/winsock/select-and-fd---2 */
#define SAFE_FD_SET(a, b) { if (a >= 0) { FD_SET(a, b); }}
#define SAFE_FD_ISSET(a, b) ((a >= 0) ? FD_ISSET(a, b) : 0)

/* You'd think that we would get better performance with a bigger buffer, but SSL frames are 16k each.
 * SSL_read in non-blocking mode will only read 16k at a time. There's no point in a bigger buffer as
 * it'd go unused.
 */
#define DPP_BUFSIZE 16 * 1024

/* Represents a failed socket system call, e.g. connect() failure */
const int ERROR_STATUS = -1;

/**
 * @brief Connect to TCP socket with a select() driven timeout
 * 
 * @param sockfd socket descriptor
 * @param addr address to connect to
 * @param addrlen address length
 * @param timeout_ms timeout in milliseconds
 * @return int -1 on error, 0 on succcess just like POSIX connect()
 * @throw dpp::connection_exception on failure
 */
int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, unsigned int timeout_ms) {
#ifdef _WIN32
	return (::connect(sockfd, addr, addrlen));
#else
	int ofcmode;
	ofcmode = fcntl(sockfd, F_GETFL, 0);
	ofcmode |= O_NDELAY;
	if (fcntl(sockfd, F_SETFL, ofcmode)) {
		throw dpp::connection_exception("Can't switch socket to non-blocking mode!");
	}
	int rc = (::connect(sockfd, addr, addrlen));
	if (rc == -1 && errno != EWOULDBLOCK && errno != EINPROGRESS) {
		throw dpp::connection_exception(strerror(errno));
	} else {
		// Set a deadline timestamp 'timeout' ms from now (needed b/c poll can be interrupted)
		double deadline = dpp::utility::time_f() + (timeout_ms / 1000.0);
		do {
			rc = -1;
			if (dpp::utility::time_f() >= deadline) {
				throw dpp::connection_exception("Connection timed out");
			}
			fd_set writefds, efds;
			FD_ZERO(&writefds);
			FD_ZERO(&efds);
			SAFE_FD_SET(sockfd, &writefds);
			SAFE_FD_SET(sockfd, &efds);
			timeval ts;
			ts.tv_sec = 0;
			ts.tv_usec = timeout_ms * 1000;
			int r = select(sockfd + 1, nullptr, &writefds, &efds, &ts);
			if (r > 0 && SAFE_FD_ISSET(sockfd, &writefds) && !SAFE_FD_ISSET(sockfd, &efds)) {
				rc = 0;
			} else if (r > 0 && SAFE_FD_ISSET(sockfd, &efds)) {
				throw dpp::connection_exception(strerror(errno));
			}
		} while (rc == -1);
	}
	ofcmode = fcntl(sockfd, F_GETFL, 0);
	ofcmode &= ~O_NDELAY;
	if (fcntl(sockfd, F_SETFL, ofcmode)) {
		throw dpp::connection_exception("Can't switch socket to blocking mode!");
	}
	return rc;
#endif
}

ssl_client::ssl_client(const std::string &_hostname, const std::string &_port, bool plaintext_downgrade, bool reuse) :
	nonblocking(false),
	sfd(INVALID_SOCKET),
	ssl(nullptr),
	last_tick(time(NULL)),
	hostname(_hostname),
	port(_port),
	bytes_out(0),
	bytes_in(0),
	plaintext(plaintext_downgrade),
	make_new(true),
	keepalive(reuse)
{
#ifndef WIN32
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGXFSZ, SIG_IGN);
#else
	// Set up winsock.
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		throw dpp::connection_exception("WSAStartup failure");
	}
#endif
	if (FD_SETSIZE < 1024) {
		throw dpp::connection_exception("FD_SETSIZE is less than 1024 (value is " + std::to_string(FD_SETSIZE) + "). This is an internal library error relating to your platform. Please report this on the official discord: https://discord.gg/dpp");
	}
	if (keepalive) {
		std::string identifier((!plaintext ? "ssl://" : "tcp://") + hostname + ":" + port);
		auto iter = keepalives.find(identifier);
		if (iter != keepalives.end()) {
			/* Found a keepalive connection, check it is still connected/valid via select for error */
			fd_set efds;
			FD_ZERO(&efds);
			SAFE_FD_SET(iter->second.sfd, &efds);
			timeval ts;
			ts.tv_sec = 0;
			ts.tv_usec = 1;
			int r = select(iter->second.sfd, nullptr, nullptr, &efds, &ts);
			if (time(nullptr) > (iter->second.created + 60) || r < 0 || FD_ISSET(iter->second.sfd, &efds)) {
				make_new = true;
				/* This connection is dead, free its resources and make a new one */
				if (iter->second.ssl->ssl) {
					SSL_free(iter->second.ssl->ssl);
					iter->second.ssl->ssl = nullptr;
				}
				if (iter->second.sfd != INVALID_SOCKET) {
					shutdown(iter->second.sfd, 2);
				}
				#ifdef _WIN32
					if (iter->second.sfd >= 0 && iter->second.sfd < FD_SETSIZE) {
						closesocket(iter->second.sfd);
					}
				#else
					::close(iter->second.sfd);
				#endif
				iter->second.sfd = INVALID_SOCKET;
				delete iter->second.ssl;
			} else {
				/* Connection is good, lets use it */
				this->sfd = iter->second.sfd;
				this->ssl = iter->second.ssl;
				make_new = false;
			}
			/* We don't keep in-flight connections in the keepalives list */
			keepalives.erase(iter);
		}

	}
	if (make_new) {
		if (plaintext) {
			ssl = nullptr;
		} else {
			ssl = new openssl_connection();
		}
	}
	this->connect();
}

/* SSL Client constructor throws std::runtime_error if it can't connect to the host */
void ssl_client::connect()
{
	/* Initial connection is done in blocking mode. There is a timeout on it. */
	nonblocking = false;

	if (make_new) {
		/* Resolve hostname to IP */
		int err = 0;
		const dns_cache_entry* addr = resolve_hostname(hostname, port);
		sfd = ::socket(addr->addr.ai_family, addr->addr.ai_socktype, addr->addr.ai_protocol);
		if (sfd == ERROR_STATUS) {
			err = errno;
		} else if (connect_with_timeout(sfd, (sockaddr*)&addr->ai_addr, (int)addr->addr.ai_addrlen, SOCKET_OP_TIMEOUT) != 0) {
#ifdef _WIN32
			if (sfd >= 0 && sfd < FD_SETSIZE) {
				closesocket(sfd);
			}
#else
			err = errno;
			shutdown(sfd, 2);
			::close(sfd);
#endif
			sfd = ERROR_STATUS;
		}

		/* Check if none of the IPs yielded a valid connection */
		if (sfd == ERROR_STATUS) {
			throw dpp::connection_exception(strerror(err));
		}

		if (!plaintext) {
			/* Each thread needs a context, but we don't need to make a new one for each connection */
			if (!openssl_context) {
				/* We're good to go - hand the fd over to openssl */
				const SSL_METHOD *method = TLS_client_method(); /* Create new client-method instance */

				/* Create SSL context */
				openssl_context = SSL_CTX_new(method);
				if (openssl_context == nullptr)
					throw dpp::connection_exception("Failed to create SSL client context!");

				/* Do not allow SSL 3.0, TLS 1.0 or 1.1
				* https://www.packetlabs.net/posts/tls-1-1-no-longer-secure/
				*/
				if (!SSL_CTX_set_min_proto_version(openssl_context, TLS1_2_VERSION))
					throw dpp::connection_exception("Failed to set minimum SSL version!");
			}

			/* Create SSL session */
			ssl->ssl = SSL_new(openssl_context);
			if (ssl->ssl == nullptr)
				throw dpp::connection_exception("SSL_new failed!");

			SSL_set_fd(ssl->ssl, (int)sfd);

			/* Server name identification (SNI) */
			SSL_set_tlsext_host_name(ssl->ssl, hostname.c_str());

#ifndef _WIN32
			/* On Linux, we can set socket timeouts so that SSL_connect eventually gives up */
			timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = SOCKET_OP_TIMEOUT * 1000;
			setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
			setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
			if (SSL_connect(ssl->ssl) != 1) {
				throw dpp::connection_exception("SSL_connect error");
			}

			this->cipher = SSL_get_cipher(ssl->ssl);
		}
	}
}

void ssl_client::write(const std::string &data)
{
	/* If we are in nonblocking mode, append to the buffer,
	 * otherwise just use SSL_write directly. The only time we
	 * use SSL_write directly is during connection before the
	 * ReadLoop is called, which allows for guaranteed simple
	 * lock-step delivery e.g. for HTTP header negotiation
	 */
	if (nonblocking) {
		obuffer += data;
	} else {
		if (plaintext) {
			if (sfd == INVALID_SOCKET || ::send(sfd, data.data(), data.length(), 0) != (int)data.length()) {
				throw dpp::connection_exception("write() failed");
			}
		} else {
			if (SSL_write(ssl->ssl, data.data(), (int)data.length()) != (int)data.length()) {
				throw dpp::connection_exception("SSL_write() failed");
			}
		}
	}
}

void ssl_client::one_second_timer()
{
}

std::string ssl_client::get_cipher() {
	return cipher;
}

void ssl_client::log(dpp::loglevel severity, const std::string &msg) const
{
}

void ssl_client::read_loop()
{
	/* The read loop is non-blocking using select(). This method
	 * cannot read while it is waiting for write, or write while it is
	 * waiting for read. This is a limitation of the openssl libraries,
	 * as SSL is sent and received in low level ~16k frames which must
	 * be synchronised and ordered correctly. Attempting to send while
	 * we need another frame or receive while we are due to send a frame
	 * would cause the protocol to break.
	 */
	int r = 0;
	size_t client_to_server_length = 0, client_to_server_offset = 0;
	bool read_blocked_on_write =  false, write_blocked_on_read = false, read_blocked = false;
	fd_set readfds, writefds, efds;
	char client_to_server_buffer[DPP_BUFSIZE], server_to_client_buffer[DPP_BUFSIZE];

	try {

		if (sfd == INVALID_SOCKET)  {
			throw dpp::connection_exception("Invalid file descriptor in read_loop()");
		}
		
		/* Make the socket nonblocking */
#ifdef _WIN32
		u_long mode = 1;
		int result = ioctlsocket(sfd, FIONBIO, &mode);
		if (result != NO_ERROR)
			throw dpp::connection_exception("Can't switch socket to non-blocking mode!");
#else
		int ofcmode = fcntl(sfd, F_GETFL, 0);
		ofcmode |= O_NDELAY;
		if (fcntl(sfd, F_SETFL, ofcmode)) {
			throw dpp::connection_exception("Can't switch socket to non-blocking mode!");
		}
#endif
		nonblocking = true;

		/* Loop until there is a socket error */
		while(true) {

			if (last_tick != time(nullptr)) {
				this->one_second_timer();
				last_tick = time(nullptr);
			}

			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_ZERO(&efds);

			SAFE_FD_SET(sfd,&readfds);
			SAFE_FD_SET(sfd,&efds);
			if (custom_readable_fd && custom_readable_fd() >= 0) {
				int cfd = (int)custom_readable_fd();
				SAFE_FD_SET(cfd, &readfds);
				SAFE_FD_SET(cfd, &efds);
			}
			if (custom_writeable_fd && custom_writeable_fd() >= 0) {
				int cfd = (int)custom_writeable_fd();
				SAFE_FD_SET(cfd, &writefds);
			}

			if (sfd == -1) {
				throw dpp::connection_exception("File descriptor invalidated, connection died");
			}

			/* If we're waiting for a read on the socket don't try to write to the server */
			if (client_to_server_length || obuffer.length() || read_blocked_on_write) {
				SAFE_FD_SET(sfd,&writefds);
			}

			timeval ts;
			ts.tv_sec = 1;
			ts.tv_usec = 0;
			r = select(FD_SETSIZE, &readfds, &writefds, &efds, &ts);
			if (r == 0)
				continue;

			if (custom_writeable_fd && custom_writeable_fd() >= 0 && SAFE_FD_ISSET(custom_writeable_fd(), &writefds)) {
				custom_writeable_ready();
			}
			if (custom_readable_fd && custom_readable_fd() >= 0 && SAFE_FD_ISSET(custom_readable_fd(), &readfds)) {
				custom_readable_ready();
			}
			if (SAFE_FD_ISSET(sfd, &efds) || sfd == INVALID_SOCKET) {
				throw dpp::connection_exception(strerror(errno));
			}

			/* Now check if there's data to read */
			if((SAFE_FD_ISSET(sfd,&readfds) && !write_blocked_on_read) || (read_blocked_on_write && SAFE_FD_ISSET(sfd,&writefds))) {
				if (plaintext) {
					read_blocked_on_write = false;
					read_blocked = false;
					r = ::recv(sfd, server_to_client_buffer, DPP_BUFSIZE, 0);
					if (r <= 0) {
						/* error or EOF */
						return;
					} else {
						buffer.append(server_to_client_buffer, r);
						if (!this->handle_buffer(buffer)) {
							return;
						}
						bytes_in += r;
					}
				} else {
					do {
						read_blocked_on_write = false;
						read_blocked = false;
						
						r = SSL_read(ssl->ssl,server_to_client_buffer,DPP_BUFSIZE);
						int e = SSL_get_error(ssl->ssl,r);

						switch (e) {
							case SSL_ERROR_NONE:
								/* Data received, add it to the buffer */
								if (r > 0) {
									buffer.append(server_to_client_buffer, r);
									if (!this->handle_buffer(buffer)) {
										return;
									}
									bytes_in += r;
								}
							break;
							case SSL_ERROR_ZERO_RETURN:
								/* End of data */
								SSL_shutdown(ssl->ssl);
								return;
							break;
							case SSL_ERROR_WANT_READ:
								read_blocked = true;
							break;
									
							/* We get a WANT_WRITE if we're trying to rehandshake and we block on a write during that rehandshake.
							* We need to wait on the socket to be writeable but reinitiate the read when it is
							*/
							case SSL_ERROR_WANT_WRITE:
								read_blocked_on_write = true;
							break;
							default:
								return;
							break;
						}

						/* We need a check for read_blocked here because SSL_pending() doesn't work properly during the
						* handshake. This check prevents a busy-wait loop around SSL_read()
						*/
					} while (SSL_pending(ssl->ssl) && !read_blocked);
				}
			}

			/* Check for input on the sendq */
			if (obuffer.length() && client_to_server_length == 0) {
				memcpy(&client_to_server_buffer, obuffer.data(), obuffer.length() > DPP_BUFSIZE ? DPP_BUFSIZE : obuffer.length());
				client_to_server_length = obuffer.length() > DPP_BUFSIZE ? DPP_BUFSIZE : obuffer.length();
				obuffer = obuffer.substr(client_to_server_length, obuffer.length());
				client_to_server_offset = 0;
			}

			/* If the socket is writeable... */
			if ((SAFE_FD_ISSET(sfd,&writefds) && client_to_server_length) || (write_blocked_on_read && SAFE_FD_ISSET(sfd,&readfds))) {
				write_blocked_on_read = false;
				/* Try to write */

				if (plaintext) {
					r = ::send(sfd, client_to_server_buffer + client_to_server_offset, (int)client_to_server_length, 0);

					if (r < 0) {
						/* Write error */
						return;
					} else {
						client_to_server_length -= r;
						client_to_server_offset += r;
						bytes_out += r;
					}
				} else {
					r = SSL_write(ssl->ssl, client_to_server_buffer + client_to_server_offset, (int)client_to_server_length);
					
					switch(SSL_get_error(ssl->ssl,r)) {
						/* We wrote something */
						case SSL_ERROR_NONE:
							client_to_server_length -= r;
							client_to_server_offset += r;
							bytes_out += r;
						break;
							
						/* We would have blocked */
						case SSL_ERROR_WANT_WRITE:
						break;
				
						/* We get a WANT_READ if we're trying to rehandshake and we block onwrite during the current connection.
						* We need to wait on the socket to be readable but reinitiate our write when it is
						*/
						case SSL_ERROR_WANT_READ:
							write_blocked_on_read = true;
						break;
								
						/* Some other error */
						default:
							return;
						break;
					}
				}
			}
		}
	}
	catch (const std::exception &e) {
		log(ll_warning, std::string("Read loop ended: ") + e.what());
	}
}

uint64_t ssl_client::get_bytes_out()
{
	return bytes_out;
}

uint64_t ssl_client::get_bytes_in()
{
	return bytes_in;
}

bool ssl_client::handle_buffer(std::string &buffer)
{
	return true;
}

void ssl_client::close()
{
	if (keepalive && this->sfd != INVALID_SOCKET) {
		std::string identifier((!plaintext ? "ssl://" : "tcp://") + hostname + ":" + port);
		auto iter = keepalives.find(identifier);
		if (iter == keepalives.end()) {
			keepalive_cache_t kc;
			kc.created = time(nullptr);
			kc.sfd = this->sfd;
			kc.ssl = this->ssl;
			keepalives.emplace(identifier, kc);
		}
		return;
	}

	if (!plaintext && ssl->ssl) {
		SSL_free(ssl->ssl);
		ssl->ssl = nullptr;
	}
	shutdown(sfd, 2);
	#ifdef _WIN32
		if (sfd >= 0 && sfd < FD_SETSIZE) {
			closesocket(sfd);
		}
	#else
		::close(sfd);
	#endif
	sfd = INVALID_SOCKET;
	obuffer.clear();
	buffer.clear();
}

ssl_client::~ssl_client()
{
	this->close();
	if (!keepalive) {
		delete ssl;
	}
}

};
