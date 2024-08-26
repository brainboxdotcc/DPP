#pragma once

namespace dpp
{
	/**
	 * @brief Represents a socket file descriptor.
	 * This is used to ensure parity between windows and unix-like systems.
	 */
#ifndef _WIN32
	using socket = int;
#else
	using socket = SOCKET;
#endif
} // namespace dpp

#ifndef SOCKET_ERROR
/**
 * @brief Represents a socket in error state
 */
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
/**
 * @brief Represents a socket which is not yet assigned
 */
#define INVALID_SOCKET ~0
#endif
