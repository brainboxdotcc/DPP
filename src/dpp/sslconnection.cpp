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
	#include <netinet/tcp.h>
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
#include <chrono>
#include <dpp/cluster.h>
#include <dpp/dns.h>
#include <dpp/ssl_context.h>
#include <dpp/wrapped_ssl_ctx.h>

namespace dpp {

/**
 * @brief Unique ID of next socket (for end-user use)
 */
uint64_t last_unique_id{1};

/**
 * @brief This is an opaque class containing openssl library specific structures.
 * We define it this way so that the public facing D++ library doesn't require
 * the openssl headers be available to build against it.
 */
class openssl_connection {
public:
	/**
	 * @brief OpenSSL context
	 */
	SSL_CTX* ctx{nullptr};
	/**
	 * @brief OpenSSL session
	 */
	SSL* ssl{nullptr};

	~openssl_connection() = default;
};

/**
 * @brief Keepalive cache record
 */
struct keepalive_cache_t {
	time_t created;
	openssl_connection* ssl;
	dpp::socket sfd;
};

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
	const int enable{1};
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
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&enable), sizeof(int));
	return true;
}

/**
 * @brief Start connecting to a TCP socket.
 * This simply calls connect() and checks for error return, as the timeout is now handled in the main
 * IO events for the ssl_connection class.
 * 
 * @param sockfd socket descriptor
 * @param addr address to connect to
 * @param addrlen address length
 * @param timeout_ms timeout in milliseconds
 * @return int -1 on error, 0 on success just like POSIX connect()
 * @throw dpp::connection_exception on failure
 */
int start_connecting(dpp::socket sockfd, const struct sockaddr *addr, socklen_t addrlen) {
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
/**
 * @brief Some old Linux and UNIX variants (BSDs) can raise signals for socket
 * errors, such as SIGPIPE etc. We filter these out so we can just concern ourselves
 * with the return codes from the functions instead.
 *
 * @note If there is an existing signal handler, it will be preserved.
 *
 * @param signal Signal code to override
 */
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

uint64_t ssl_connection::get_unique_id() const {
	return unique_id;
}

ssl_connection::ssl_connection(cluster* creator, const std::string &_hostname, const std::string &_port, bool plaintext_downgrade, bool reuse) :
	is_server(false),
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
		detail::wrapped_ssl_ctx* context = detail::generate_ssl_context();
		ssl->ctx = context->context;
	}
	try {
		ssl_connection::connect();
	}
	catch (std::exception&) {
		cleanup();
		throw;
	}
}

ssl_connection::ssl_connection(cluster* creator, socket fd, uint16_t port, bool plaintext_downgrade, const std::string& private_key, const std::string& public_key) :
	is_server(true),
	sfd(fd),
	ssl(nullptr),
	last_tick(time(nullptr)),
	start(time(nullptr)),
	bytes_out(0),
	bytes_in(0),
	plaintext(plaintext_downgrade),
	timer_handle(0),
	unique_id(last_unique_id++),
	keepalive(false),
	owner(creator),
	private_key_file(private_key),
	public_key_file(public_key)
{
	if (plaintext) {
		ssl = nullptr;
	} else {
		ssl = new openssl_connection();
		detail::wrapped_ssl_ctx* context = detail::generate_ssl_context(port, private_key, public_key);
		ssl->ctx = context->context;
	}

	if (!set_nonblocking(sfd, true)) {
		throw dpp::connection_exception(err_nonblocking_failure, "Can't switch socket to non-blocking mode!");
	}
}

void ssl_connection::on_buffer_drained() {
}

/* SSL Client constructor throws std::runtime_error if it can't allocate a socket or call connect() */
void ssl_connection::connect() {
	/* Resolve hostname to IP */
	int err = 0;
	const dns_cache_entry* addr = resolve_hostname(hostname, port);
	sfd = addr->make_connecting_socket();
	address_t destination = addr->get_connecting_address(from_string<uint16_t>(this->port, std::dec));
	if (sfd == ERROR_STATUS) {
		err = errno;
	} else {
		start_connecting(sfd, destination.get_socket_address(), destination.size());
	}
	/* Check if valid connection started */
	if (sfd == ERROR_STATUS) {
		throw dpp::connection_exception(err_connect_failure, strerror(err));
	}
}

void ssl_connection::socket_write(const std::string_view data) {
	/* Because this is a non-blocking system we never write immediately. We append to the buffer,
	 * which writes later.
	 */
	std::lock_guard<std::mutex> lock(out_mutex);
	obuffer += data;
	owner->socketengine->inplace_modify_fd(sfd, WANT_WRITE);
}

void ssl_connection::one_second_timer() {
}

std::string ssl_connection::get_cipher() {
	return cipher;
}

void ssl_connection::log(dpp::loglevel severity, const std::string &msg) const {
}

void ssl_connection::complete_handshake(const socket_events* ev)
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
		do_raw_trace("(SSL): <complete handshake>");
		socket_events se{*ev};
		se.flags = dpp::WANT_WRITE | dpp::WANT_READ | dpp::WANT_ERROR;
		owner->socketengine->update_socket(se);
		connected = true;
		this->cipher = SSL_get_cipher(ssl->ssl);
	}

}

void ssl_connection::do_raw_trace(const std::string& message) const {
	if (raw_trace) {
		log(ll_trace, "RAWTRACE" + message);
	}
}

void ssl_connection::on_read(socket fd, const struct socket_events& ev) {

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
		do_raw_trace("(IN,PLAIN): " + std::string(server_to_client_buffer, r));
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
					do_raw_trace("(IN,SSL): " + std::string(server_to_client_buffer, r));
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

	{
		std::lock_guard<std::mutex> lock(out_mutex);
		if (connected && ssl && ssl->ssl && (!obuffer.empty() || SSL_want_write(ssl->ssl))) {
			socket_events se{ev};
			se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
			owner->socketengine->update_socket(se);
		}
	}
}

void ssl_connection::on_write(socket fd, const struct socket_events& e) {

	if (sfd == INVALID_SOCKET) {
		return;
	}

	if (!tcp_connect_done) {
		/* The first write event on an outbound TCP socket indicates connect() has finished */
		tcp_connect_done = true;
		do_raw_trace("(OUT): <TCP connect() done>");
	}
	if (!connected && plaintext) {
		/* Plaintext sockets connect immediately on first write event.
		 * There is nothing more to do, so set connected to true.
		 */
		connected = true;
	} else if (!connected) {
		/* SSL handshake and session setup. SSL sessions require more legwork
		 * to get them initialised after connect() completes. We do that here.
		 */
		if (ssl != nullptr && ssl->ctx != nullptr && ssl->ssl == nullptr) {
			/* Now we can create SSL session.
			 * These are unique to each connection, using the context.
			 */
			ssl->ssl = SSL_new(ssl->ctx);
			if (ssl->ssl == nullptr) {
				throw dpp::connection_exception(err_ssl_new, "SSL_new failed!");
			}

			/* Associate the SSL session with the file descriptor, and set it as connecting */
			SSL_set_fd(ssl->ssl, (int) sfd);
			if (this->is_server) {
				SSL_set_accept_state(ssl->ssl);
			} else {
				SSL_set_connect_state(ssl->ssl);
				/* Server name identification (SNI)
				 * This is needed for modern HTTPS and tells SSL which virtual host to connect a
				 * socket to: https://www.cloudflare.com/en-gb/learning/ssl/what-is-sni/
				 */
				SSL_set_tlsext_host_name(ssl->ssl, hostname.c_str());
			}
		}

		/* If this completes, we fall straight through into if (connected) */
		complete_handshake(&e);
	}

	if (connected) {
		{
			/* We are fully connected, check if we have output buffer to send */
			std::lock_guard<std::mutex> lock(out_mutex);
			if (!obuffer.empty() && client_to_server_length == 0) {
				/* If we do, copy it to the raw buffer OpenSSL uses */
				memcpy(&client_to_server_buffer, obuffer.data(), obuffer.length() > DPP_BUFSIZE ? DPP_BUFSIZE : obuffer.length());
				client_to_server_length = obuffer.length() > DPP_BUFSIZE ? DPP_BUFSIZE : obuffer.length();
				obuffer = obuffer.substr(client_to_server_length, obuffer.length());
				client_to_server_offset = 0;
			}
		}

		if (plaintext) {
			/* For plaintext connections life is simple, we just call ::send() to send as much of the buffer as we can */
			if (client_to_server_length > 0) {
				int r = static_cast<int>(::send(sfd, client_to_server_buffer + client_to_server_offset, static_cast<int>(client_to_server_length), 0));
				do_raw_trace("(OUT,PLAIN): " + std::string(client_to_server_buffer + client_to_server_offset, client_to_server_length));
				if (r < 0) {
					/* Write error */
					do_raw_trace("(OUT,PLAIN): <error>");
					this->close();
					return;
				}
				/* We wrote some, or all of the buffer */
				client_to_server_length -= r;
				client_to_server_offset += r;
				bytes_out += r;
				if (obuffer.empty()) {
					on_buffer_drained();
				}
			} else {
				/* Spurious write event for empty buffer */
				do_raw_trace("(OUT,PLAIN): <NOT WRITING EMPTY BUFFER>");
			}
			{
				std::lock_guard<std::mutex> lock(out_mutex);
				if (!obuffer.empty()) {
					/* Still content to send? Request that we get a write event */
					socket_events se{e};
					se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
					do_raw_trace("(OUT,PLAIN): <MORE BUFFER REMAINS>");
					owner->socketengine->update_socket(se);
				}
			}
		} else if (ssl && ssl->ssl) {
			/* SSL connections are more complex, the SSL_write function can return some weirdness. See below. */
			int err{SSL_ERROR_NONE};
			int r{0};
			if (client_to_server_length > 0) {
				r = SSL_write(ssl->ssl, client_to_server_buffer + client_to_server_offset, static_cast<int>(client_to_server_length));
				do_raw_trace("(OUT,SSL): " + std::string(client_to_server_buffer + client_to_server_offset, client_to_server_length));
				err = SSL_get_error(ssl->ssl, r);
			} else {
				/* Spurious write event for empty buffer */
				do_raw_trace("(OUT,SSL): <NOT WRITING EMPTY BUFFER>");
			}

			/* Handle SSL_write return code */
			switch (err) {
				case SSL_ERROR_NONE: {
					/* We wrote some or all of the buffer */
					std::lock_guard<std::mutex> lock(out_mutex);
					client_to_server_length -= r;
					client_to_server_offset += r;
					bytes_out += r;
					if (!obuffer.empty()) {
						/* Still content to send? Request that we get a write event */
						socket_events se{e};
						se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
						owner->socketengine->update_socket(se);
						do_raw_trace("(OUT,SSL): <MORE BUFFER REMAINS>");
					} else {
						on_buffer_drained();
					}
					do_raw_trace("(OUT,SSL): <OK>");
					break;
				}
				case SSL_ERROR_WANT_READ: {
					/* OpenSSL said we wrote, but now it wants a read event */
					socket_events se{e};
					se.flags = WANT_READ | WANT_ERROR;
					owner->socketengine->update_socket(se);
					do_raw_trace("(OUT,SSL): <WANT READ>");
					break;
				}
				case SSL_ERROR_WANT_WRITE: {
					/* OpenSSL said it still needs another write event */
					socket_events se{e};
					se.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
					owner->socketengine->update_socket(se);
					do_raw_trace("(OUT,SSL): <WANT WRITE>");
					break;
				}
				case SSL_ERROR_SYSCALL: {
					/* There was an actual error */
					do_raw_trace("(OUT,SSL): <SYSCALL ERROR>");
					if (errno != 0) {
						/* If errno != 0, it was a socket error, close socket */
						this->close();
					}
					break;
				}
				/* Some other error, these are not valid here, so we do nothing! */
				default: {
					return;
				}
			}
		}
	}
}

void ssl_connection::on_error(socket fd, const struct socket_events&, int error_code) {
	this->close();
}

void ssl_connection::read_loop() {
	auto setup_events = [this]() {
		dpp::socket_events events(
			sfd,
			WANT_READ | WANT_WRITE | WANT_ERROR,
			[this](socket fd, const struct socket_events &e) {
				if (this->sfd == INVALID_SOCKET) {
					close_socket(fd);
					owner->socketengine->delete_socket(fd);
					return;
				}
				on_read(fd, e);
			},
			[this](socket fd, const struct socket_events &e) {
				if (this->sfd == INVALID_SOCKET) {
					close_socket(fd);
					owner->socketengine->delete_socket(fd);
					return;
				}
				on_write(fd, e);
			},
			[this](socket fd, const struct socket_events &e, int error_code) {
				do_raw_trace("on_error");
				on_error(fd, e, error_code);
			}
		);
		owner->socketengine->register_socket(events);
	};
	setup_events();
	if (!timer_handle) {
		timer_handle = owner->start_timer([this, setup_events](auto handle) {
			one_second_timer();
			if (!tcp_connect_done && time(nullptr) > start + 2 && connect_retries < MAX_RETRIES && sfd != INVALID_SOCKET) {
				/* Retry failed connect(). This can happen even in the best situation with bullet-proof hosting.
				 * Previously with blocking connect() there was some leniency in this, but now we have to do this
				 * ourselves.
				 *
				 * Retry up to 3 times, 2 seconds between retries. After this, give up and let timeout code
				 * take the wheel (will likely end with an exception).
				 */
				do_raw_trace("(OUT) connect() retry #" + std::to_string(connect_retries + 1));
				close_socket(sfd);
				owner->socketengine->delete_socket(sfd);
				try {
					ssl_connection::connect();
				}
				catch (const std::exception& e) {
					do_raw_trace("(OUT): connect() exception: " + std::string(e.what()));
				}
				setup_events();
				start = time(nullptr) + 2;
				connect_retries++;
			}
		}, 1);
	}
}

uint64_t ssl_connection::get_bytes_out() {
	return bytes_out;
}

uint64_t ssl_connection::get_bytes_in() {
	return bytes_in;
}

bool ssl_connection::handle_buffer(std::string &buffer) {
	return true;
}

void ssl_connection::close() {
	/**
	 * Many of the values here are reset to initial values in the case
	 * we want to reconnect the socket after closing it. This is not something
	 * that is done often.
	 */
	if (!plaintext) {
		if (ssl != nullptr && ssl->ssl != nullptr) {
			SSL_free(ssl->ssl);
			ssl->ssl = nullptr;
		}
	}
	connected = tcp_connect_done = false;
	client_to_server_length = client_to_server_offset = 0;
	last_tick = time(nullptr);
	bytes_in = bytes_out = 0;
	if (sfd != INVALID_SOCKET) {
		log(ll_trace, "ssl_connection::close() with sfd");
		owner->socketengine->delete_socket(sfd);
		close_socket(sfd);
		sfd = INVALID_SOCKET;
	}
	obuffer.clear();
	buffer.clear();
}

void ssl_connection::cleanup() {
	this->close();
}

ssl_connection::~ssl_connection() {
	cleanup();
	if (timer_handle) {
		owner->stop_timer(timer_handle);
		timer_handle = 0;
	}
	delete ssl;
	ssl = nullptr;
}

}
