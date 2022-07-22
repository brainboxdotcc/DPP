// Formatting library for C++ - optional OS-specific functionality
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

// Disable bogus MSVC warnings.
#if !defined(_CRT_SECURE_NO_WARNINGS) && defined(_MSC_VER)
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <climits>

#if FMT_USE_FCNTL
#  include <sys/stat.h>
#  include <sys/types.h>

#  ifndef _WIN32
#    include <unistd.h>
#  else
#    ifndef WIN32_LEAN_AND_MEAN
#      define WIN32_LEAN_AND_MEAN
#    endif
#    include <io.h>

#    ifndef S_IRUSR
#      define S_IRUSR _S_IREAD
#    endif
#    ifndef S_IWUSR
#      define S_IWUSR _S_IWRITE
#    endif
#    ifndef S_IRGRP
#      define S_IRGRP 0
#    endif
#    ifndef S_IROTH
#      define S_IROTH 0
#    endif
#  endif  // _WIN32
#endif    // FMT_USE_FCNTL

#ifdef _WIN32
#  include <windows.h>
#endif

#ifdef fileno
#  undef fileno
#endif

namespace {
#ifdef _WIN32
// Return type of read and write functions.
using rwresult = int;

// On Windows the count argument to read and write is unsigned, so convert
// it from size_t preventing integer overflow.
inline unsigned convert_rwcount(std::size_t count) {
  return count <= UINT_MAX ? static_cast<unsigned>(count) : UINT_MAX;
}
#elif FMT_USE_FCNTL
// Return type of read and write functions.
using rwresult = ssize_t;

inline std::size_t convert_rwcount(std::size_t count) { return count; }
#endif
}  // namespace
