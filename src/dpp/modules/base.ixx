module;

#include "modules.h"

#if !DPP_IMPORT_STD

#include <string>
#include <exception>
#include <algorithm>
#include <charconv>
#include <type_traits>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <algorithm> // all_of, find, for_each
#include <cstddef> // nullptr_t, ptrdiff_t, size_t
#include <functional> // hash, less
#include <initializer_list> // initializer_list
#include <iosfwd> // istream, ostream
#include <iterator> // random_access_iterator_tag
#include <memory> // unique_ptr
#include <numeric> // accumulate
#include <string> // string, stoi, to_string
#include <utility> // declval, forward, move, pair, swap
#include <vector> // vector

#ifdef DPP_FORMATTERS
#include <format>
#endif

#endif

#include "dpp/json.h"

export module dpp:base;

#if DPP_IMPORT_STD

import std;

#endif

#include "dpp/dpp_fwd.h"
#include "dpp/json_fwd.h"
#include "dpp/exception.h"
#include "dpp/snowflake.h"
