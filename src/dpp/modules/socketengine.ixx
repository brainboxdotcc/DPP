module;

#include "modules.h"

#ifdef _WIN32
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
	#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
	#define pollfd WSAPOLLFD
#else
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif

#include <ctime>

#if !DPP_IMPORT_STD

#include <string>
#include <unordered_map>
#include <cstring>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <string_view>
#include <functional>
#include <shared_mutex>
#include <string>
#include <functional>
#include <mutex>

#endif

#include "dpp/json.h"

export module dpp:socketengine;

#if DPP_IMPORT_STD

import std;

#endif

import :base;
import :utility;

#include "dpp/bignum.h"
#include "dpp/socket.h"
#include "dpp/dns.h"
#include "dpp/socketengine.h"
