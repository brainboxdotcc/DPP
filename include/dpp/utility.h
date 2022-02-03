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
#include <dpp/misc-enum.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>

/**
 * @brief The main namespace for D++ functions. classes and types
 */
namespace dpp {

	/**
	 * @brief Utility helper functions, generally for logging
	 */
	namespace utility {

		/**
		 * @brief Timestamp formats for dpp::utility::timestamp()
		 * 
		 * @note These values are the actual character values specified by the Discord API
		 * and should not be changed unless the Discord API changes the specification!
		 * They have been sorted into numerical order of their ASCII value to keep C++ happy.
		 */
		enum time_format : uint8_t {
			/// "20 April 2021" - Long Date
			tf_long_date		=	'D',
			/// "Tuesday, 20 April 2021 16:20" - Long Date/Time
			tf_long_datetime	=	'F',
			/// "2 months ago" - Relative Time		
			tf_relative_time	=	'R',
			/// "16:20:30" - Long Time
			tf_long_time		=	'T',
			/// "20/04/2021" - Short Date
			tf_short_date		=	'd',
			/// "20 April 2021 16:20" - Short Date/Time
			tf_short_datetime	=	'f',
			/// "16:20" - Short Time
			tf_short_time		=	't',
		};

		/**
		 * @brief The base URL for CDN content such as profile pictures and guild icons.
		 */
		const std::string cdn_host = "https://cdn.discordapp.com"; 

		/**
		 * @brief Callback for the results of a command executed via dpp::utility::exec
		 */
		typedef std::function<void(const std::string& output)> cmd_result_t;

		/**
		 * @brief Run a commandline program asynchronously. The command line program
		 * is spawned in a separate std::thread, and when complete, its output from
		 * stdout is passed to the callback function in its string parameter. For example
		 * ```
		 * dpp::utility::exec("/bin/ls", {"-al"}, [](const std::string& output) {
		 *     std::cout << "Output of 'ls -al': " << output << "\n";
		 * });
		 * ```
		 * 
		 * @param cmd The command to run.
		 * @param parameters Command line parameters. Each will be escaped using `std::quoted`.
		 * @param callback The callback to call on completion.
		 */
		void DPP_EXPORT exec(const std::string& cmd, std::vector<std::string> parameters = {}, cmd_result_t callback = {});

		/**
		 * @brief Return a mentionable timestamp (used in a discord embed)
		 * 
		 * @param ts Time stamp to convert
		 * @param tf Format of timestamp using dpp::utility::time_format
		 * @return std::string 
		 */
		std::string DPP_EXPORT timestamp(time_t ts, time_format tf);

		/**
		 * @brief Returns current date and time
		 * 
		 * @return std::string Current date and time
		 */
		std::string DPP_EXPORT current_date_time();
		/**
		 * @brief Convert a dpp::loglevel enum value to a string
		 * 
		 * @param in log level to convert
		 * @return std::string string form of log level
		 */
		std::string DPP_EXPORT loglevel(dpp::loglevel in);

		/**
		 * @brief Store a 128 bit icon hash (profile picture, server icon etc)
		 * as a 128 bit binary value made of two uint64_t.
		 * Has a constructor to build one from a string, and a method to fetch
		 * the value back in string form.
		 */
		struct DPP_EXPORT iconhash {

			uint64_t first;		//!< High 64 bits
			uint64_t second;	//!< Low 64 bits

			/**
			 * @brief Construct a new iconhash object
			 */
			iconhash();

			/**
			 * @brief Construct a new iconhash object
			 * 
			 * @param hash String hash to construct from.
			 * Must contain a 32 character hex string.
			 * 
			 * @throws std::length_error if the provided
			 * string is not exactly 32 characters long.
			 */
			iconhash(const std::string &hash);

			/**
			 * @brief Assign from std::string
			 * 
			 * @param assignment string to assign from.
			 * 
			 * @throws std::length_error if the provided
			 * string is not exactly 32 characters long.
			 */
			iconhash& operator=(const std::string &assignment);

			/**
			 * @brief Change value of iconhash object
			 * 
			 * @param hash String hash to change to.
			 * Must contain a 32 character hex string.
			 * 
			 * @throws std::length_error if the provided
			 * string is not exactly 32 characters long.
			 */
			void set(const std::string &hash);

			/**
			 * @brief Convert iconhash back to 32 character
			 * string value.
			 * 
			 * @return std::string Hash value 
			 */
			std::string to_string() const;
		};

		/**
		 * @brief Return the current time with fractions of seconds.
		 * This is a unix epoch time with the fractional seconds part
		 * after the decimal place.
		 * 
		 * @return double time with fractional seconds
		 */
		double DPP_EXPORT time_f();

		/**
		 * @brief Returns true if D++ was built with voice support
		 * 
		 * @return bool True if voice support is compiled in (libsodium/libopus) 
		 */
		bool DPP_EXPORT has_voice();

		/**
		 * @brief Convert a byte count to display value
		 * 
		 * @param c number of bytes
		 * @return std::string display value suffixed with M, G, T where necessary
		 */
		std::string DPP_EXPORT bytes(uint64_t c);

		/**
		 * @brief A class used to represent an uptime in hours, minutes,
		 * seconds and days, with helper functions to convert from time_t
		 * and display as a string.
		 */
		struct DPP_EXPORT uptime {
			uint16_t days;	//!< Number of days
			uint8_t hours;	//!< Number of hours
			uint8_t mins;	//!< Number of minutes
			uint8_t secs;	//!< Number of seconds

			/**
			 * @brief Construct a new uptime object
			 */
			uptime();

			/**
			 * @brief Construct a new uptime object
			 * 
			 * @param diff A time_t to initialise the object from
			 */
			uptime(time_t diff);

			/**
			 * @brief Get uptime as string
			 * 
			 * @return std::string Uptime as string
			 */
			std::string to_string();

			/**
			 * @brief Get uptime as seconds
			 * 
			 * @return uint64_t Uptime as seconds
			 */
			uint64_t to_secs();

			/**
			 * @brief Get uptime as milliseconds
			 * 
			 * @return uint64_t Uptime as milliseconds
			 */
			uint64_t to_msecs();
		};

		/**
		 * @brief Convert floats to RGB for sending in embeds
		 * 
		 * @param red red value, between 0 and 1 inclusive
		 * @param green green value, between 0 and 1 inclusive
		 * @param blue blue value, between 0 and 1 inclusive
		 * @return uint32_t returned integer colour value
		 */
		uint32_t rgb(float red, float green, float blue);

		/**
		 * @brief Convert ints to RGB for sending in embeds
		 * 
		 * @param red red value, between 0 and 255 inclusive
		 * @param green green value, between 0 and 255 inclusive
		 * @param blue blue value, between 0 and 255 inclusive
		 * @return uint32_t returned integer colour value
		 */
		uint32_t rgb(int red, int green, int blue);

		/**
		 * @brief Output hex values of a section of memory for debugging
		 * 
		 * @param data The start of the data to display
		 * @param length The length of data to display
		 */
		std::string DPP_EXPORT debug_dump(uint8_t* data, size_t length);

		/**
		 * @brief Returns the length of a UTF-8 string in codepoints
		 * 
		 * @param str string to count length of
		 * @return size_t length of string (0 for invalid utf8)
		 */
		size_t DPP_EXPORT utf8len(const std::string &str);

		/**
		 * @brief Return substring of a UTF-8 encoded string in codepoints
		 * 
		 * @param str string to return substring from
		 * @param start start codepoint offset
		 * @param length length in codepoints
		 * @return std::string Substring in UTF-8 or empty string if invalid UTF-8 passed in
		 */
		std::string DPP_EXPORT utf8substr(const std::string& str, std::string::size_type start, std::string::size_type length);

		/**
		 * @brief Read a whole file into a std::string.
		 * Be sure you have enough memory to read the file, if you are reading a large file.
		 * @note Be aware this function can block! If you are regularly reading large files, consider caching them.
		 * @param filename The path to the file to read
		 * @return std::string The file contents
		 * @throw dpp::exception on failure to read the entire file
		 */
		std::string DPP_EXPORT read_file(const std::string& filename);

		/**
		 * @brief Validate a string value
		 * In the event the length of the string is less than _min, then an exception of type dpp:length_exception
		 * will be thrown. If the string is longer than _max UTF8 codepoints it will be truncated to fit.
		 * 
		 * @param value The value to validate
		 * @param _min Minimum length
		 * @param _max Maximum length
		 * @param exception_message Exception message to throw if value length < _min
		 * @return std::string Validated string, truncated if necessary.
		 * @throw dpp::length_exception if value UTF8 length < _min
		 */
		std::string validate(const std::string& value, size_t _min, size_t _max, const std::string& exception_message);

		/**
		 * @brief Return the url parameter for an avatar size, or empty if the size is 0
		 * 
		 * @param size size to generate url parameter for
		 * @return std::string url parameter
		 */
		std::string avatar_size(uint32_t size);
	};

};
