/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <dpp/user.h>
#include <dpp/channel.h>
#include <dpp/cluster.h>
#include <utility>


namespace dpp {
/**
 * @brief Return end of message
 * @return 0
*/
int end_msg();
/**
 * @brief Simple parent for streams
 * 
 */

class DPP_EXPORT base_stream {
    public:
        /**
         * @brief Build message in stream
         * @param msg Text to add
         * @return This stream, for purposes of chaining
         */
        base_stream& operator<<(const std::string& msg);
        /**
         * @brief Send message in stream
         * @param n Number
         * @return This stream, for purposes of chaining
         */
        base_stream& operator<<(const int& n);
        //TODO Make action rows work with this
        /**
         * @brief Build message in stream
         * @param c Component to add
         * @return This stream, for purposes of chaining
         */
        base_stream& operator<<(const component &c);
        /**
         * @brief Build message in stream
         * @param e Embed to add
         * @return This stream, for purposes of chaining
         */
        base_stream& operator<<(const embed &e);
        /**
         * @brief Build message in stream
         * @param f Pair of filename and filecontent to add
         * @return This stream, for purposes of chaining
         */
        base_stream& operator<<(const std::pair<std::string,std::string> &f);
        virtual void send(command_completion_event_t callback = utility::log_error()) {};
        virtual message send_sync() { return message(); };
        message msg = message();
        
};
/**
 * @brief A stream wrapper to send messages
 */ 
class DPP_EXPORT dm_stream : public base_stream {
	public:
		/**
		 * @brief Constructer
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel User to whom messages will be sent
		 */
		dm_stream(cluster& bot, user out_user); 
		/**
		 * @brief Constructer
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel_id User id to whom messages will be sent
		 */
		dm_stream(cluster& bot, snowflake out_user_id);
		/**
		 * @brief Send message with stream
		 * @param callback Function to call when the API call completes. On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
		 */
	
		void send(command_completion_event_t callback = utility::log_error()) override;
		/**
		 * @brief Send message synchronously
		 * @return Message sent
		 * @warning This function is a blocking (synchronous) call and should only be used from within a separate thread.
 		 * Avoid direct use of this function inside an event handler.
		 */
		message send_sync() override;
		
	private:	
		cluster* bot; /**< Cluster which sends messages */
		snowflake out_user_id; /**< User id to which messages are sent */

};
/**
 * @brief A stream wrapper to send messages
 */ 
class DPP_EXPORT channel_stream : public base_stream {
	public:
		/**
		 * @brief Constructer
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel Channel where messages will be sent
		 */
		channel_stream(cluster& bot, channel out_channel); 
		/**
		 * @brief Constructer
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel_id Channel id where messages will be sent
		 */
		channel_stream(cluster& bot, snowflake out_channel_id);
		/**
		 * @brief Send message with stream
		 * @param msg Message to send
		 * @param callback Function to call when the API call completes. On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
		 */
	
		void send(command_completion_event_t callback = utility::log_error()) override;
		/**
		 * @brief Send message synchronously
		 * @param msg Message to send
		 * @return Message sent
		 * @warning This function is a blocking (synchronous) call and should only be used from within a separate thread.
 		 * Avoid direct use of this function inside an event handler.
		 */
		message send_sync() override;
		
	private:	
		cluster* bot; /**< Cluster which sends messages */
		snowflake out_channel_id; /**< Channel id in which messages are sent */

};

}