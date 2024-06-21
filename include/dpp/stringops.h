/************************************************************************************
 * 
 * D++ - A Lightweight C++ Library for Discord
 *
 * stringops.h taken from TriviaBot
 *
 * Copyright 2004 Craig Edwards <support@sporks.gg>
 *
 * Core based on Sporks, the Learning Discord Bot, Craig Edwards (c) 2019.
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
#include <string>
#include <iomanip>
#include <locale>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <charconv>

#ifdef __cpp_lib_spanstream
#include <spanstream>
#endif

namespace dpp {

#ifndef __cpp_lib_spanstream
/**
 * @brief streambuf for imemstream
 * @note From https://stackoverflow.com/a/13059195
 */
struct membuf : std::streambuf
{
    membuf(const char* base, size_t size)
    {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};

/**
 * @brief An input stream over a raw character array. Superseded by std::ispanstream in C++23
 * @note From https://stackoverflow.com/a/13059195
 */
struct imemstream : virtual membuf, std::istream
{
    imemstream(const char* base, size_t size)
        : membuf(base, size), std::istream(static_cast<std::streambuf*>(this)) {}
};
#endif

/**
 * @brief Convert a string to lowercase using tolower()
 * 
 * @tparam T type of string
 * @param s String to lowercase
 * @return std::basic_string<T> lowercased string
 */
template <typename T> std::basic_string<T> lowercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2(s);
	std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
	return s2;
}

/**
 * @brief Convert a string to lowercase using tolower()
 *
 * @tparam T type of string
 * @param s String_view to lowercase
 * @return std::basic_string<T> lowercased string
 */
template <typename T> std::basic_string<T> lowercase(std::basic_string_view<T> s)
{
    std::basic_string<T> s2(s);
    std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
    return s2;
}

/**
 * @brief Convert a string to uppercase using toupper()
 * 
 * @tparam T type of string
 * @param s String to uppercase
 * @return std::basic_string<T> uppercased string 
 */
template <typename T> std::basic_string<T> uppercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2(s);
	std::transform(s2.begin(), s2.end(), s2.begin(), toupper);
	return s2;
}

/**
 * @brief Convert a string to uppercase using toupper()
 *
 * @tparam T type of string
 * @param s String_view to uppercase
 * @return std::basic_string<T> uppercased string
 */
template <typename T> std::basic_string<T> uppercase(std::basic_string_view<T> s)
{
    std::basic_string<T> s2(s);
    std::transform(s2.begin(), s2.end(), s2.begin(), toupper);
    return s2;
}

/**
 * @brief trim from end of string (right)
 * 
 * @param s String to trim
 * @return std::string trimmed string
 */
inline std::string rtrim(std::string_view s)
{
    std::string s_cpy(s);
    s_cpy.erase(s_cpy.find_last_not_of(" \t\n\r\f\v") + 1);
    return s_cpy;
}

/**
 * @brief trim from beginning of string (left)
 * 
 * @param s string to trim
 * @return std::string trimmed string
 */
inline std::string ltrim(std::string_view s)
{
    std::string s_cpy(s);
    s_cpy.erase(0, s_cpy.find_first_not_of(" \t\n\r\f\v"));
    return s_cpy;
}

/**
 * @brief Trim from both ends of string (right then left)
 * 
 * @param s string to trim 
 * @return std::string trimmed string
 */
inline std::string trim(std::string_view s)
{
	return ltrim(rtrim(s));
}

/**
 * @brief Add commas to a string (or dots) based on current locale server-side
 * 
 * @tparam T type of numeric value
 * @param value Value
 * @return std::string number with commas added 
 */
template<class T> std::string comma(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

/**
 * @brief  Convert any value from a string to another type using stringstream.
 * The optional second parameter indicates the format of the input string,
 * e.g. std::dec for decimal, std::hex for hex, std::oct for octal.
 * 
 * @tparam T Type to convert to 
 * @param s String to convert from
 * @param f Numeric base, e.g. `std::dec` or `std::hex`
 * @return T Returned numeric value
 */
template <typename T> T from_string(std::string_view s, std::ios_base & (*f)(std::ios_base&))
{
	T t;

#ifdef __cpp_lib_spanstream
    std::span<const char> span(s.begin(), s.end());
    std::ispanstream iss(span);
    iss >> f, iss >> t;
#else
    imemstream ims(s.data(), s.size());
    ims >> f, ims >> t;
#endif

	return t;
}

/**
 * @brief  Convert any value from a string to another type using stringstream.
 * 
 * @tparam T Type to convert to 
 * @param s String to convert from
 * @return T Returned numeric value
 *
 * @note Base 10 for numeric conversions.
 */
template <typename T> T from_string(std::string_view s)
{
	if (s.empty()) {
		return static_cast<T>(0);
	}

	T t;

#ifdef __cpp_lib_spanstream
    std::span<const char> span(s.begin(), s.end());
    std::ispanstream iss(span);
    iss >> t;
#else
    imemstream ims(s.data(), s.size());
    ims >> t;
#endif

	return t;
}

/**
 * @brief Specialised conversion of uint64_t from string
 * 
 * @tparam int64_t 
 * @param s string to convert 
 * @return uint64_t return value
 */
template <uint64_t> uint64_t from_string(std::string_view s)
{
    uint64_t value{};
    std::from_chars(s.data(), s.data() + s.size(), value);
    return value;
}

/**
 * @brief Specialised conversion of uint32_t from string
 * 
 * @tparam uint32_t 
 * @param s string to convert
 * @return uint32_t return value
 */
template <uint32_t> uint32_t from_string(std::string_view s)
{
    uint32_t value{};
    std::from_chars(s.data(), s.data() + s.size(), value);
    return value;
}

/**
 * @brief Specialised conversion of int from string
 * 
 * @tparam int 
 * @param s string to convert
 * @return int return value
 */
template <int> int from_string(std::string_view s)
{
    int value{};
    std::from_chars(s.data(), s.data() + s.size(), value);
    return value;
}

/**
 * @brief Convert a numeric value to hex
 * 
 * @tparam T numeric type
 * @param i numeric value
 * @param leading_zeroes set to false if you don't want the leading zeroes in the output
 * @return std::string value in hex, the length will be 2* the raw size of the type
 */
template <typename T> std::string to_hex(T i, bool leading_zeroes = true)
{
	char str[26] = { 0 };
	size_t size = sizeof(T) * 2;
	std::to_chars(std::begin(str), std::end(str), i, 16);
	std::string out{str};
	if (leading_zeroes && out.length() < size) {
		out.insert(out.begin(), size - out.length(), '0');
	}
	return out;
}

/**
 * @brief Format a numeric type as a string with leading zeroes
 * 
 * @tparam T numeric type
 * @param i numeric value
 * @param width width of type including the leading zeroes
 * @return std::string resultant string with leading zeroes
 */
template <typename T> std::string leading_zeroes(T i, size_t width)
{
	std::stringstream stream;
	stream.imbue(std::locale::classic());
	stream << std::setfill('0') << std::setw((int)width) << std::dec << i;
	return stream.str();
}

} // namespace dpp
