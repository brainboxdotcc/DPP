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
#pragma once
#include <dpp/export.h>
#include <dpp/misc-enum.h>
#include <string>
#include <functional>
#include <ctime>
#include <mutex>
#include <dpp/socket.h>
#include <cstdint>
#include <dpp/timer.h>

namespace dpp {

/**
 * @brief This is an opaque class containing openssl library specific structures.
 * We define it this way so that the public facing D++ library doesn't require
 * the openssl headers be available to build against it.
 */
class openssl_connection;

/**
 * @brief Close a socket 
 * 
 * @param sfd Socket to close
 * @return false on error, true on success
 */
DPP_EXPORT bool close_socket(dpp::socket sfd);

/**
 * @brief Set a socket to blocking or non-blocking IO
 *
 * @param sockfd socket to act upon
 * @param non_blocking should socket be non-blocking?
 * @return false on error, true on success
 */
DPP_EXPORT bool set_nonblocking(dpp::socket sockfd, bool non_blocking);

/**
 * @brief SSL_read buffer size
 *
 * You'd think that we would get better performance with a bigger buffer, but SSL frames are 16k each.
 * SSL_read in non-blocking mode will only read 16k at a time. There's no point in a bigger buffer as
 * it'd go unused.
 */
constexpr uint16_t DPP_BUFSIZE{16 * 1024};

/**
 * @brief Represents a failed socket system call, e.g. connect() failure
 */
constexpr int ERROR_STATUS{-1};

/**
 * @brief Maximum number of internal connect() retries on TCP connections
 */
constexpr int MAX_RETRIES{4};

/**
 * @brief Implements a simple non-blocking SSL stream connection.
 */
class DPP_EXPORT ssl_connection
{
private:
	/**
	 * @brief Clean up resources
	 */
	void cleanup();

	/**
	 * @brief Mutex for creation of internal SSL pointers by openssl
	 */
	std::mutex ssl_mutex;

	/**
	 * @brief Mutex for output buffer
	 */
	std::mutex out_mutex;

	/**
	 * @brief Start offset into internal ring buffer for client to server IO
	 */
	size_t client_to_server_length = 0;

	/**
	 * @brief Start offset into internal ring buffer for server to client IO
	 */
	size_t client_to_server_offset = 0;

	/**
	 * @brief Internal ring buffer for client to server IO
	 */
	char client_to_server_buffer[DPP_BUFSIZE];

	/**
	 * @brief Internal ring buffer for server to client IO
	 */
	char server_to_client_buffer[DPP_BUFSIZE];

	/**
	 * @brief True if this connection is a server inbound connection from accept()
	 */
	bool is_server = false;

protected:
	/**
	 * @brief Input buffer received from socket
	 */
	std::string buffer;

	/**
	 * @brief Output buffer for sending to socket
	 */
	std::string obuffer;

	/**
	 * @brief Raw file descriptor of connection
	 */
	dpp::socket sfd;

	/**
	 * @brief Openssl opaque contexts
	 */
	openssl_connection* ssl;

	/**
	 * @brief SSL cipher in use
	 */
	std::string cipher;

	/**
	 * @brief For timers
	 */
	time_t last_tick;

	/**
	 * @brief Start time of connection
	 */
	time_t start;

	/**
	 * @brief How many times we retried connect()
	 */
	uint8_t connect_retries{0};

	/**
	 * @brief Hostname connected to
	 */
	std::string hostname;

	/**
	 * @brief Port connected to
	 */
	std::string port;

	/**
	 * @brief Bytes out
	 */
	uint64_t bytes_out;

	/**
	 * @brief Bytes in
	 */
	uint64_t bytes_in;

	/**
	 * @brief True for a plain text connection
	 */
	bool plaintext;

	/**
	 * @brief True if connection is completed
	 */
	bool connected{false};

	/**
	 * @brief True if tcp connect() succeeded
	 */
	bool tcp_connect_done{false};

	/**
	 * @brief Timer handle for one second timer
	 */
	timer timer_handle;

	/**
	 * @brief Unique ID of socket used as a nonce
	 * You can use this to identify requests vs reply
	 * if you want. D++ itself only sets this, and does
	 * not use it in any logic. It starts at 1 and increments
	 * for each request made.
	 */
	uint64_t unique_id;

	/**
	 * @brief Called every second
	 */
	virtual void one_second_timer();

	/**
	 * @brief Start SSL connection and connect to TCP endpoint
	 * @throw dpp::exception Failed to initialise connection
	 */
	virtual void connect();

	/**
	 * @brief Set this to true to log all IO to debug for this connection.
	 * This is an internal developer facility. Do not enable it unless you
	 * need to, as it will be very noisy.
	 */
	bool raw_trace{false};

	/**
	 * @brief If raw_trace is set to true, log a debug message for this connection
	 * @param message debug message
	 */
	void do_raw_trace(const std::string& message) const;

	virtual void on_buffer_drained();

public:
	/**
	 * @brief For low-level debugging, calling this function will
	 * enable low level I/O logging for this connection to the logger.
	 * This can be very loud, and output a lot of data, so only enable it
	 * selectively where you need it.
	 *
	 * Generally, you won't need this, it is a library development utility.
	 */
	void enable_raw_tracing();

	/**
	 * @brief Get the bytes out objectGet total bytes sent
	 * @return uint64_t bytes sent
	 */
	uint64_t get_bytes_out();
	
	/**
	 * @brief Get total bytes received
	 * @return uint64_t bytes received
	 */
	uint64_t get_bytes_in();

	/**
	 * @brief Every request made has a unique ID. This increments
	 * for every request, starting at 1. You can use this for statistics,
	 * or to associate requests and replies in external event loops.
	 * @return Unique ID
	 */
	uint64_t get_unique_id() const;

	/**
	 * @brief Get SSL cipher name
	 * @return std::string ssl cipher name
	 */
	std::string get_cipher();

	/**
	 * @brief True if we are keeping the connection alive after it has finished
	 */
	bool keepalive;

	/**
	 * @brief Owning cluster
	 */
	class cluster* owner;

	/**
	 * @brief Private key PEM file path for inbound SSL connections
	 */
	std::string private_key_file;

	/**
	 * @brief Public key PEM file path for inbound SSL connections
	 */
	std::string public_key_file;

	/**
	 * @brief Connect to a specified host and port. Throws std::runtime_error on fatal error.
	 * @param creator Creating cluster
	 * @param _hostname The hostname to connect to
	 * @param _port the Port number to connect to
	 * @param plaintext_downgrade Set to true to connect using plaintext only, without initialising SSL.
	 * @param reuse Attempt to reuse previous connections for this hostname and port, if available
	 * Note that no Discord endpoints will function when downgraded. This option is provided only for
	 * connection to non-Discord addresses such as within dpp::cluster::request().
	 * @throw dpp::exception Failed to initialise connection
	 */
	ssl_connection(cluster* creator, const std::string &_hostname, const std::string &_port = "443", bool plaintext_downgrade = false, bool reuse = false);

	/**
	 * @brief Accept a new connection from listen()/accept() socket
	 * @param creator Creating cluster
	 * @param fd Socket file descriptor assigned by accept()
	 * @param port Port the new fd came from
	 * @param plaintext_downgrade Set to true to connect using plaintext only, without initialising SSL.
	 * @param private_key if plaintext_downgrade is set to false, a private key PEM file for SSL connections
	 * @param public_key if plaintext_downgrade is set to false, a public key PEM file for SSL connections
	 */
	ssl_connection(cluster* creator, socket fd, uint16_t port, bool plaintext_downgrade = false, const std::string& private_key = "", const std::string& public_key = "");

	/**
	 * @brief Set up non blocking I/O and configure on_read, on_write and on_error.
	 * @throw std::exception Any std::exception (or derivative) thrown from read_loop() indicates setup failed
	 */
	void read_loop();

	/**
	 * @brief Destroy the ssl_connection object
	 */
	virtual ~ssl_connection();

	/**
	 * @brief Handle input from the input buffer. This function will be called until
	 * all data in the buffer has been processed and the buffer is empty.
	 * @param buffer the buffer content. Will be modified removing any processed front elements
	 * @return bool True if the socket should remain connected
	 */
	virtual bool handle_buffer(std::string &buffer);

	/**
	 * @brief Write to the output buffer.
	 * @param data Data to be written to the buffer.
	 * @note The data may not be written immediately and may be written at a later time to the socket.
	 */
	void socket_write(const std::string_view data);

	/**
	 * @brief Close socket connection
	 */
	virtual void close();

	/**
	 * @brief Log a message
	 * @param severity severity of log message
	 * @param msg Log message to send
	 */
	virtual void log(dpp::loglevel severity, const std::string &msg) const;

	/**
	 * @brief Called while SSL handshake is in progress.
	 * If the handshake completes, the state of the socket is progressed to
	 * an established state.
	 * @param ev Socket events for the socket
	 */
	void complete_handshake(const struct socket_events* ev);

	/**
	 * @brief Called when the TCP socket has data to read
	 * @param fd File descriptor
	 * @param ev Socket events
	 */
	void on_read(dpp::socket fd, const struct dpp::socket_events& ev);

	/**
	 * @brief Called when the TCP socket can be written to without blocking
	 * @param fd File descriptor
	 * @param e Socket events
	 */
	void on_write(dpp::socket fd, const struct dpp::socket_events& e);

	/**
	 * @brief Called when there is an error on the TCP socket
	 * @param fd File descriptor
	 * @param error_code Error code
	 */
	void on_error(dpp::socket fd, const struct dpp::socket_events&, int error_code);
};

}
