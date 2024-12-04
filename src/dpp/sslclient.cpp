/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <dpp/export.h>
#include <cerrno>
#ifdef _WIN32
	/* Windows-specific sockets includes */
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
#else
	/* Anything other than Windows (e.g. sane OSes) */
	#include <sys/socket.h>
	#include <unistd.h>
#endif
#include <csignal>
#include <sys/types.h>
#include <fcntl.h>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
/* Windows specific OpenSSL symbol weirdness */
#ifdef OPENSSL_SYS_WIN32
	#undef X509_NAME
	#undef X509_EXTENSIONS
	#undef X509_CERT_PAIR
	#undef PKCS7_ISSUER_AND_SERIAL
	#undef OCSP_REQUEST
	#undef OCSP_RESPONSE
#endif
#include <exception>
#include <string>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <dpp/cluster.h>
#include <dpp/sslclient.h>
#include <dpp/exception.h>
#include <dpp/stringops.h>
#include <dpp/dns.h>
#include <dpp/socketengine.h>

/* Maximum allowed time in milliseconds for socket read/write timeouts and connect() */
constexpr uint16_t SOCKET_OP_TIMEOUT{5000};

namespace dpp {

uint64_t last_unique_id{1};

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
	SSL* ssl{nullptr};
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
 * @brief Custom deleter for SSL_CTX
 */
class openssl_context_deleter {
public:
	void operator()(SSL_CTX* context) const noexcept {
		SSL_CTX_free(context);
	}
};

/**
 * @brief OpenSSL context
 */
thread_local std::unique_ptr<SSL_CTX, openssl_context_deleter> openssl_context;

bool close_socket(dpp::socket sfd)
{
	/* close_socket on an error socket is a non-op */
	if (sfd != INVALID_SOCKET) {
		shutdown(sfd, 2);
#ifdef _WIN32
		return closesocket(sfd) == 0;
#else
		return ::close(sfd) == 0;
#endif
	}
	return false;
}

bool set_nonblocking(dpp::socket sockfd, bool non_blocking)
{
#ifdef _WIN32
	u_long mode = non_blocking ? 1 : 0;
	int result = ioctlsocket(sockfd, FIONBIO, &mode);
	if (result != NO_ERROR) {
		return false;
	}
#else
	int ofcmode = fcntl(sockfd, F_GETFL, 0);
	if (non_blocking) {
		ofcmode |= O_NDELAY;
	} else {
		ofcmode &= ~O_NDELAY;
	}
	if (fcntl(sockfd, F_SETFL, ofcmode)) {
		return false;
	}
#endif
	return true;
}

/**
 * @brief Connect to TCP socket with a poll() driven timeout
 * 
 * @param sockfd socket descriptor
 * @param addr address to connect to
 * @param addrlen address length
 * @param timeout_ms timeout in milliseconds
 * @return int -1 on error, 0 on success just like POSIX connect()
 * @throw dpp::connection_exception on failure
 */
int start_connecting(dpp::socket sockfd, const struct sockaddr *addr, socklen_t addrlen, unsigned int timeout_ms) {
	if (!set_nonblocking(sockfd, true)) {
		throw dpp::connection_exception(err_nonblocking_failure, "Can't switch socket to non-blocking mode!");
	}
#ifdef _WIN32
	/* Windows connect returns -1 and sets its error value to 0 for successfull blocking connection -
	 * This is equivalent to EWOULDBLOCK on POSIX
	 */
	ULONG non_blocking = 1;
	ioctlsocket(sockfd, FIONBIO, &non_blocking);
	int rc = WSAConnect(sockfd, addr, addrlen, nullptr, nullptr, nullptr, nullptr);
	int err = EWOULDBLOCK;
#else
	/* Standard POSIX connection behaviour */
	int rc = (::connect(sockfd, addr, addrlen));
	int err = errno;
#endif
	if (rc == -1 && err != EWOULDBLOCK && err != EINPROGRESS) {
		throw connection_exception(err_connect_failure, strerror(errno));
	}
	return 0;
}

#ifndef _WIN32
void set_signal_handler(int signal)
{
	struct sigaction sa{};
	sigaction(signal, nullptr, &sa);
	if (sa.sa_flags == 0 && sa.sa_handler == nullptr) {
		sa = {};
		sigaction(signal, &sa, nullptr);
	}
}
#endif

uint64_t ssl_client::get_unique_id() const {
	return unique_id;
}

ssl_client::ssl_client(cluster* creator, const std::string &_hostname, const std::string &_port, bool plaintext_downgrade, bool reuse) :
	sfd(INVALID_SOCKET),
	ssl(nullptr),
	last_tick(time(nullptr)),
	start(time(nullptr)),
	hostname(_hostname),
	port(_port),
	bytes_out(0),
	bytes_in(0),
	plaintext(plaintext_downgrade),
	timer_handle(0),
	unique_id(last_unique_id++),
	keepalive(reuse),
	owner(creator)
{
	if (plaintext) {
		ssl = nullptr;
	} else {
		ssl = new openssl_connection();
	}
	try {
		this->connect();
	}
	catch (std::exception&) {
		cleanup();
		throw;
	}
}

/* SSL Client constructor throws std::runtime_error if it can't connect to the host */
void ssl_client::connect()
{
	/* Resolve hostname to IP */
	int err = 0;
	const dns_cache_entry* addr = resolve_hostname(hostname, port);
	sfd = addr->make_connecting_socket();
	address_t destination = addr->get_connecting_address(from_string<uint16_t>(this->port, std::dec));
	if (sfd == ERROR_STATUS) {
		err = errno;
	} else {
		start_connecting(sfd, destination.get_socket_address(), destination.size(), SOCKET_OP_TIMEOUT);
	}
	/* Check if valid connection started */
	if (sfd == ERROR_STATUS) {
		throw dpp::connection_exception(err_connect_failure, strerror(err));
	}
}

void ssl_client::socket_write(const std::string_view data)
{
	obuffer += data;
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

void ssl_client::complete_handshake(const socket_events* ev)
{
	if (!ssl || !ssl->ssl) {
		return;
	}
	auto status = SSL_do_handshake(ssl->ssl);
	if (status != 1) {
		auto code = SSL_get_error(ssl->ssl, status);
		switch (code) {
			case SSL_ERROR_NONE: {
				connected = true;
				socket_events se{*ev};
				se.flags = dpp::WANT_READ | dpp::WANT_WRITE | dpp::WANT_ERROR;
				owner->socketengine->update_socket(se);
				break;
			}
			case SSL_ERROR_WANT_WRITE: {
				socket_events se{*ev};
				se.flags = dpp::WANT_READ | dpp::WANT_WRITE | dpp::WANT_ERROR;
				owner->socketengine->update_socket(se);
				break;
			}
			case SSL_ERROR_WANT_READ: {
				socket_events se{*ev};
				se.flags = dpp::WANT_READ | dpp::WANT_ERROR;
				owner->socketengine->update_socket(se);
				break;
			}
			default: {
				throw dpp::connection_exception(err_ssl_connect, "SSL_do_handshake error: " + std::to_string(status)  +";" + std::to_string(code));
			}
		}
	} else {
		socket_events se{*ev};
		se.flags = dpp::WANT_WRITE | dpp::WANT_READ | dpp::WANT_ERROR;
		owner->socketengine->update_socket(se);
		connected = true;
		this->cipher = SSL_get_cipher(ssl->ssl);
	}

}

void ssl_client::on_read(socket fd, const struct socket_events& ev) {

	if (sfd == INVALID_SOCKET) {
		return;
	}

	if (plaintext && connected) {
		int r = (int) ::recv(sfd, server_to_client_buffer, DPP_BUFSIZE, 0);
		if (r <= 0) {
			this->close();
			return;
		}
		buffer.append(server_to_client_buffer, r);
		if (!this->handle_buffer(buffer)) {
			this->close();
			return;
		}
		bytes_in += r;
	} else if (!plaintext && connected && ssl && ssl->ssl) {
		int r = SSL_read(ssl->ssl, server_to_client_buffer, DPP_BUFSIZE);
		int e = SSL_get_error(ssl->ssl,r);

		switch (e) {
			case SSL_ERROR_NONE:
				/* Data received, add it to the buffer */
				if (r > 0) {
					buffer.append(server_to_client_buffer, r);

					if (!this->handle_buffer(buffer)) {
						this->close();
						return;
					} else {
						socket_events se{ev};
						se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
						owner->socketengine->update_socket(se);
					}
					bytes_in += r;
				}
				break;
			case SSL_ERROR_ZERO_RETURN:
				/* End of data */
				SSL_shutdown(ssl->ssl);
				return;
			case SSL_ERROR_WANT_READ: {
				socket_events se{ev};
				se.flags = WANT_READ | WANT_ERROR;
				owner->socketengine->update_socket(se);
				break;
			}
			/* We get a WANT_WRITE if we're trying to rehandshake, and we block on a write during that rehandshake.
			 * We need to wait on the socket to be writeable but initiate the read when it is
			 */
			case SSL_ERROR_WANT_WRITE: {
				socket_events se{ev};
				se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
				owner->socketengine->update_socket(se);
				break;
			}
			case SSL_ERROR_SYSCALL: {
				if (errno != 0) {
					this->close();
				}
				break;
			}
			default: {
				this->close();
				return;
			}
		}
	}

	if (!connected && !plaintext) {
		complete_handshake(&ev);
	}

	if (connected && ssl && ssl->ssl && (!obuffer.empty() || SSL_want_write(ssl->ssl))) {
		socket_events se{ev};
		se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
		owner->socketengine->update_socket(se);
	}
}

void ssl_client::on_write(socket fd, const struct socket_events& e) {

	if (sfd == INVALID_SOCKET) {
		return;
	}

	if (!tcp_connect_done) {
		tcp_connect_done = true;
	}
	if (!connected && plaintext) {
		/* Plaintext sockets connect immediately on first write event */
		connected = true;
	} else if (!connected) {
		/* SSL handshake and session setup */

		/* Each thread needs a context, but we don't need to make a new one for each connection */
		if (!openssl_context) {
			/* We're good to go - hand the fd over to openssl */
			const SSL_METHOD *method = TLS_client_method(); /* Create new client-method instance */

			/* Create SSL context */
			openssl_context.reset(SSL_CTX_new(method));
			if (!openssl_context) {
				throw dpp::connection_exception(err_ssl_context, "Failed to create SSL client context!");
			}

			/* Do not allow SSL 3.0, TLS 1.0 or 1.1
			* https://www.packetlabs.net/posts/tls-1-1-no-longer-secure/
			*/
			if (!SSL_CTX_set_min_proto_version(openssl_context.get(), TLS1_2_VERSION)) {
				throw dpp::connection_exception(err_ssl_version, "Failed to set minimum SSL version!");
			}
		}
		if (ssl != nullptr && ssl->ssl == nullptr) {
			/* Create SSL session */
			std::lock_guard<std::mutex> lock(ssl_mutex);
			ssl->ssl = SSL_new(openssl_context.get());
			if (ssl->ssl == nullptr) {
				throw dpp::connection_exception(err_ssl_new, "SSL_new failed!");
			}

			SSL_set_fd(ssl->ssl, (int) sfd);
			SSL_set_connect_state(ssl->ssl);

			/* Server name identification (SNI) */
			SSL_set_tlsext_host_name(ssl->ssl, hostname.c_str());
		}

		/* If this completes, we fall straight through into if (connected) */
		complete_handshake(&e);
	}

	if (connected) {
		if (obuffer.length() && client_to_server_length == 0) {
			memcpy(&client_to_server_buffer, obuffer.data(), obuffer.length() > DPP_BUFSIZE ? DPP_BUFSIZE : obuffer.length());
			client_to_server_length = obuffer.length() > DPP_BUFSIZE ? DPP_BUFSIZE : obuffer.length();
			obuffer = obuffer.substr(client_to_server_length, obuffer.length());
			client_to_server_offset = 0;
		}

		if (plaintext) {
			int r = (int) ::send(sfd, client_to_server_buffer + client_to_server_offset, (int)client_to_server_length, 0);
			if (r < 0) {
				/* Write error */
				this->close();
				return;
			}
			client_to_server_length -= r;
			client_to_server_offset += r;
			bytes_out += r;
			if (client_to_server_length > 0) {
				socket_events se{e};
				se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
				owner->socketengine->update_socket(se);
			}
		} else if (ssl && ssl->ssl) {
			int r = SSL_write(ssl->ssl, client_to_server_buffer + client_to_server_offset, (int)client_to_server_length);
			int err = SSL_get_error(ssl->ssl, r);

			switch (err) {
				/* We wrote something */
				case SSL_ERROR_NONE:
					client_to_server_length -= r;
					client_to_server_offset += r;
					bytes_out += r;
					break;
					/* We would have blocked */
				case SSL_ERROR_WANT_READ: {
					socket_events se{e};
					se.flags = WANT_READ | WANT_ERROR;
					owner->socketengine->update_socket(se);
					break;
				}
				case SSL_ERROR_WANT_WRITE: {
					socket_events se{e};
					se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
					owner->socketengine->update_socket(se);
					break;
				}
				case SSL_ERROR_SYSCALL: {
					if (errno != 0) {
						this->close();
					}
					break;
				}
				/* Some other error */
				default: {
					return;
				}
			}
		}
	}
}

void ssl_client::on_error(socket fd, const struct socket_events&, int error_code) {
	if (sfd != INVALID_SOCKET) {
		this->close();
	}
}

void ssl_client::read_loop()
{
	auto setup_events = [this]() {
		dpp::socket_events events(
			sfd,
			WANT_READ | WANT_WRITE | WANT_ERROR,
			[this](socket fd, const struct socket_events &e) { on_read(fd, e); },
			[this](socket fd, const struct socket_events &e) { on_write(fd, e); },
			[this](socket fd, const struct socket_events &e, int error_code) { on_error(fd, e, error_code); }
		);
		owner->socketengine->register_socket(events);
	};
	setup_events();
	if (!timer_handle) {
		timer_handle = owner->start_timer([this, setup_events](auto handle) {
			one_second_timer();
			if (!tcp_connect_done && time(nullptr) > start + 2 && connect_retries < MAX_RETRIES) {
				/* Retry failed connect(). This can happen even in the best situation with bullet-proof hosting.
				 * Previously with blocking connect() there was some leniency in this, but now we have to do this
				 * ourselves.
				 *
				 * Retry up to 3 times, 2 seconds between retries. After this, give up and let timeout code
				 * take the wheel (will likely end with an exception).
				 */
				close_socket(sfd);
				owner->socketengine->delete_socket(sfd);
				ssl_client::connect();
				setup_events();
				start = time(nullptr) + 2;
				connect_retries++;
			}
		}, 1);
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
	if (!plaintext && ssl->ssl) {
		std::lock_guard<std::mutex> lock(ssl_mutex);
		SSL_free(ssl->ssl);
		ssl->ssl = nullptr;
	}
	connected = tcp_connect_done = false;
	client_to_server_length = client_to_server_offset = 0;
	last_tick = time(nullptr);
	bytes_in = bytes_out = 0;
	owner->socketengine->delete_socket(sfd);
	close_socket(sfd);
	sfd = INVALID_SOCKET;
	obuffer.clear();
	buffer.clear();
}

void ssl_client::cleanup()
{
	this->close();
}

ssl_client::~ssl_client()
{
	cleanup();
	if (timer_handle) {
		owner->stop_timer(timer_handle);
		timer_handle = 0;
	}
	delete ssl;
	ssl = nullptr;
}

}
