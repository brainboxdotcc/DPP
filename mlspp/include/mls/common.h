#pragma once

#include <array>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

// Expose the bytes library globally
#include <bytes/bytes.h>
using namespace mlspp::bytes_ns;

// Expose the compatibility library globally
#include <tls/compat.h>
namespace var = mlspp::tls::var;
namespace opt = mlspp::tls::opt;

namespace mlspp {

// Make variant equality work in the same way as optional equality, with
// automatic unwrapping.  In other words
//
//     v == T(x) <=> hold_alternative<T>(v) && get<T>(v) == x
//
// For consistency, we also define symmetric and negated version.  In this
// house, we obey the symmetric law of equivalence relations!
template<typename T, typename... Ts>
bool
operator==(const var::variant<Ts...>& v, const T& t)
{
  return var::visit(
    [&](const auto& arg) {
      using U = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<U, T>) {
        return arg == t;
      } else {
        return false;
      }
    },
    v);
}

template<typename T, typename... Ts>
bool
operator==(const T& t, const var::variant<Ts...>& v)
{
  return v == t;
}

template<typename T, typename... Ts>
bool
operator!=(const var::variant<Ts...>& v, const T& t)
{
  return !(v == t);
}

template<typename T, typename... Ts>
bool
operator!=(const T& t, const var::variant<Ts...>& v)
{
  return !(v == t);
}

using epoch_t = uint64_t;

///
/// Get the current system clock time in the format MLS expects
///

uint64_t
seconds_since_epoch();

///
/// Easy construction of overloaded lambdas
///

template<class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;

  // XXX(RLB) MSVC has a bug where it incorrectly computes the size of this
  // type.  Microsoft claims they have fixed it in the latest MSVC, and GitHub
  // claims they are running a version with the fix.  But in practice, we still
  // hit it.  Including this dummy variable is a work-around.
  //
  // https://developercommunity.visualstudio.com/t/runtime-stack-corruption-using-stdvisit/346200
  int dummy = 0;
};

// clang-format off
// XXX(RLB): For some reason, different versions of clang-format disagree on how
// this should be formatted.  Probably because it's new syntax with C++17?
// Exempting it from clang-format for now.
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
// clang-format on

///
/// Auto-generate equality and inequality operators for TLS-serializable things
///

template<typename T>
inline typename std::enable_if<T::_tls_serializable, bool>::type
operator==(const T& lhs, const T& rhs)
{
  return lhs._tls_fields_w() == rhs._tls_fields_w();
}

template<typename T>
inline typename std::enable_if<T::_tls_serializable, bool>::type
operator!=(const T& lhs, const T& rhs)
{
  return lhs._tls_fields_w() != rhs._tls_fields_w();
}

///
/// Error types
///

// The `using parent = X` / `using parent::parent` construction here
// imports the constructors of the parent.

class NotImplementedError : public std::exception
{
public:
  using parent = std::exception;
  using parent::parent;
};

class ProtocolError : public std::runtime_error
{
public:
  using parent = std::runtime_error;
  using parent::parent;
};

class IncompatibleNodesError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

class InvalidParameterError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

class InvalidPathError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

class InvalidIndexError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

class InvalidMessageTypeError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

class MissingNodeError : public std::out_of_range
{
public:
  using parent = std::out_of_range;
  using parent::parent;
};

class MissingStateError : public std::out_of_range
{
public:
  using parent = std::out_of_range;
  using parent::parent;
};

// A slightly more elegant way to silence -Werror=unused-variable
template<typename T>
void
silence_unused(const T& val)
{
  (void)val;
}

namespace stdx {

// XXX(RLB) This method takes any container in, but always puts the resuls in
// std::vector.  The output could be made generic with a Rust-like syntax,
// defining a PendingTransform object that caches the inputs, with a template
// `collect()` method that puts them in an output container.  Which makes the
// calling syntax as follows:
//
//   auto out = stdx::transform(in, f).collect<Container>();
//
// (You always need the explicit specialization, even if assigning it to an
// explicitly typed variable, because C++ won't infer return types.)
//
// Given that the above syntax is pretty chatty, and we never need anything
// other than vectors here anyway, I have left this as-is.
template<typename Value, typename Container, typename UnaryOperation>
std::vector<Value>
transform(const Container& c, const UnaryOperation& op)
{
  auto out = std::vector<Value>{};
  auto ins = std::inserter(out, out.begin());
  std::transform(c.begin(), c.end(), ins, op);
  return out;
}

template<typename Container, typename UnaryPredicate>
bool
any_of(const Container& c, const UnaryPredicate& pred)
{
  return std::any_of(c.begin(), c.end(), pred);
}

template<typename Container, typename UnaryPredicate>
bool
all_of(const Container& c, const UnaryPredicate& pred)
{
  return std::all_of(c.begin(), c.end(), pred);
}

template<typename Container, typename UnaryPredicate>
auto
count_if(const Container& c, const UnaryPredicate& pred)
{
  return std::count_if(c.begin(), c.end(), pred);
}

template<typename Container, typename Value>
bool
contains(const Container& c, const Value& val)
{
  return std::find(c.begin(), c.end(), val) != c.end();
}

template<typename Container, typename UnaryPredicate>
auto
find_if(Container& c, const UnaryPredicate& pred)
{
  return std::find_if(c.begin(), c.end(), pred);
}

template<typename Container, typename UnaryPredicate>
auto
find_if(const Container& c, const UnaryPredicate& pred)
{
  return std::find_if(c.begin(), c.end(), pred);
}

template<typename Container, typename Value>
auto
upper_bound(const Container& c, const Value& val)
{
  return std::upper_bound(c.begin(), c.end(), val);
}

} // namespace stdx

} // namespace mlspp
