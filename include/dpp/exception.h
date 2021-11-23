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
#include <string>
#include <exception>
#include <algorithm>

namespace dpp {

/**
 * @brief The dpp::exception class derives from std::exception and supports some other
 * ways of passing in error details such as via std::string.
 */
class exception : public std::exception
{
protected:
	/**
	 * @brief Exception message
	 */
	std::string msg;

public:

	using std::exception::exception;

	/**
	 * @brief Construct a new exception object
	 */
	exception() = default;

	/**
	 * @brief Construct a new exception object
	 * 
	 * @param what reason message
	 */
	explicit exception(const char* what) : msg(what) { }

	/**
	 * @brief Construct a new exception object
	 * 
	 * @param what reason message
	 * @param len length of reason message
	 */
	exception(const char* what, size_t len) : msg(what, len) { }

	/**
	 * @brief Construct a new exception object
	 * 
	 * @param what reason message
	 */
	explicit exception(const std::string& what) : msg(what) { }
	
	/**
	 * @brief Construct a new exception object
	 * 
	 * @param what reason message
	 */
	explicit exception(std::string&& what) : msg(std::move(what)) { }

	/**
	 * @brief Construct a new exception object (copy constructor)
	 */
	exception(const exception&) = default;

	/**
	 * @brief Construct a new exception object (move constructor)
	 */
	exception(exception&&) = default;

	/**
	 * @brief Destroy the exception object
	 */
	~exception() override = default;

	/**
	 * @brief Copy assignment operator
	 * 
	 * @return exception& reference to self
	 */
	exception & operator = (const exception &) = default;

	/**
	 * @brief Move assignment operator
	 * 
	 * @return exception& reference to self
	 */
	exception & operator = (exception&&) = default;

	/**
	 * @brief Get exception message
	 * 
	 * @return const char* error message
	 */
	[[nodiscard]] const char* what() const noexcept override { return msg.c_str(); };

};

#ifndef _DOXYGEN_
	#define derived_exception(name, ancestor) class name : public ancestor { \
	public: \
		using dpp::exception::exception; \
		name() = default; \
		explicit name(const char* what) : exception(what) { } \
		name(const char* what, size_t len) : exception(what, len) { } \
		explicit name(const std::string& what) : exception(what) { } \
		explicit name(std::string&& what) : exception(what) { } \
		name(const name&) = default; \
		name(name&&) = default; \
		~name() override = default; \
		name & operator = (const name &) = default; \
		name & operator = (name&&) = default; \
		[[nodiscard]] const char* what() const noexcept override { return msg.c_str(); }; \
	};
#endif

#ifdef _DOXYGEN_
	/*
	 * THESE DEFINITIONS ARE NOT THE REAL DEFINITIONS USED BY PROGRAM CODE.
	 *
	 * They exist only to cause Doxygen to emit proper documentation for the derived exception types.
	 * Proper definitions are emitted by the `derived_exception` macro in the "else" section.
	 */

	/**
	 * @brief Represents an error in logic, e.g. you asked the library to do something the Discord API does not support
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class logic_exception : public dpp::exception { };
	/**
	 * @brief Represents an error reading or writing to a file
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class file_exception : public dpp::exception { };
	/**
	 * @brief Represents an error establishing or maintaining a connection
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class connection_exception : public dpp::exception { };
	/**
	 * @brief Represents an error with voice processing
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class voice_exception : public dpp::exception { };
	/**
	 * @brief Represents an error on a REST API call, e.g. a HTTPS request
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class rest_exception : public dpp::exception { };
	/**
	 * @brief Represents invalid length of argument being passed to a function
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class length_exception : public dpp::exception { };
	/**
	 * @brief Represents inability to parse data, usually caused by malformed JSON or ETF
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class parse_exception : public dpp::exception { };
	/**
	 * @brief Represents invalid access to dpp's cache or its members, which may or may not exist. 
	 * @note This is a stub for documentation purposes. For full information on supported methods please see dpp::exception.
	 */
	class cache_exception : public dpp::exception { };
#else
	derived_exception(logic_exception, dpp::exception);
	derived_exception(file_exception, dpp::exception);
	derived_exception(connection_exception, dpp::exception);
	derived_exception(voice_exception, dpp::exception);
	derived_exception(rest_exception, dpp::exception);
	derived_exception(length_exception, dpp::exception);
	derived_exception(parse_exception, dpp::exception);
	derived_exception(cache_exception, dpp::exception);
#endif

};

