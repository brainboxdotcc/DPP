#pragma once

#ifndef _WIN32
#ifndef SOCKET
#define SOCKET int
#endif
#endif

namespace dpp
{
    typedef SOCKET socket;
}

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET ~0
#endif