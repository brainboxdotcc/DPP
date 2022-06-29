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
#include <dpp/export.h>
#include <errno.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <io.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#include <sys/types.h>
#include <exception>
#include <string>
#include <dpp/exception.h>

namespace dpp {

	/**
	 * @brief Resolve a hostname to an addrinfo
	 * 
	 * @param hostname Hostname to resolve
	 * @param port A port number or named service, e.g. "80"
	 * @return addrinfo First IP address associated with the hostname DNS record
	 */
	addrinfo DPP_EXPORT resolve_hostname(const std::string& hostname, const std::string& port);
};
