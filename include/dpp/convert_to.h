#include <cstring>
#include <array>
#include <memory>
#include <new>

/**
 * @brief Safely convert one type to another without UB.
 * This is produces identical assembly to reinterpret_cast.
 *
 * @param ptr Input pointer type
 * @return Converted pointer type
 *
 * @tparam IN source type
 * @tparam OUT destination type
 *
 * This is essentially std::start_lifetime_as(), without
 * the requirement for C++23.
 *
 * @note Based on code by Krystian Stasiowski
 * (https://github.com/sdkrystian)
 */
template<typename IN, typename OUT> OUT* convert_to(IN* ptr)
{
	return std::launder(static_cast<OUT*>(std::memmove(ptr, ptr, sizeof(OUT))));
}
