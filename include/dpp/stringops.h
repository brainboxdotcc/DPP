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

/**
 * @brief Convert a string to lowercase using tolower()
 * 
 * @tparam T type of string
 * @param s String to lowercase
 * @return std::basic_string<T> lowercased string
 */
template <typename T> std::basic_string<T> lowercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2 = s;
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
    std::basic_string<T> s2 = s;
    std::transform(s2.begin(), s2.end(), s2.begin(), toupper);
    return s2;
}

/**
 * @brief trim from end of string (right)
 * 
 * @param s String to trim
 * @return std::string trimmed string
 */
inline std::string rtrim(std::string s)
{
	s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
	return s;
}

/**
 * @brief trim from beginning of string (left)
 * 
 * @param s string to trim
 * @return std::string trimmed string
 */
inline std::string ltrim(std::string s)
{
	s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
	return s;
}

/**
 * @brief Trim from both ends of string (right then left)
 * 
 * @param s string to trim 
 * @return std::string trimmed string
 */
inline std::string trim(std::string s)
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
template<class T> std::string Comma(T value)
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
template <typename T> T from_string(const std::string &s, std::ios_base & (*f)(std::ios_base&))
{
	T t;
	std::istringstream iss(s);
	iss >> f, iss >> t;
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
template <typename T> T from_string(const std::string &s)
{
	T t;
	std::istringstream iss(s);
	iss >> t;
	return t;
}

template <uint64_t> uint64_t from_string(const std::string &s)
{
	return std::stoull(s, 0, 10);
}

template <uint32_t> uint32_t from_string(const std::string &s)
{
	return std::stoul(s, 0, 10);
}

template <int> int from_string(const std::string &s)
{
	return std::stoi(s, 0, 10);
}
