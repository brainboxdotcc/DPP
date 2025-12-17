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
#pragma once

#ifdef _WIN32
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
	namespace dpp::compat {
		using pollfd = WSAPOLLFD;
		inline int poll(pollfd* fds, ULONG nfds, int timeout) {
			return WSAPoll(fds, nfds, timeout);
		}
	} // namespace dpp::compat
	#pragma comment(lib, "ws2_32")
#else
	#include <poll.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	namespace dpp::compat {
		using ::pollfd;
		using ::poll;
	} // namespace dpp::compat
#endif
