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
 * @brief Structure to act as special end of message
 * 
 */
struct send_msg_t {};

/**
 * @brief Structure to act as special end of message
 * 
 */
struct add_row_t {};

/**
 * @brief Simple wrapper for file info
 * 
 */
struct file_info {
	std::string file_name;
	std::string file_content;
	std::string mime_type;
};
/**
 * @brief End and send a message in stream
 * 
 * @return send_msg_t Simple send message object
 */
send_msg_t send_msg();
/**
 * @brief End a row in stream
 * 
 * @return add_row_t Simple add row object
 */
add_row_t add_row(); 
/**
 * @brief Simple parent for streams
 * 
 */
class DPP_EXPORT message_builder {
    public:
        /**
         * @brief Build message in stream
         * @param msg Text to add
         * @return This stream, for purposes of chaining
         */
        message_builder& operator<<(const std::string& msg);
        /**
         * @brief Finish and send message in stream, argument indicates end of message 
         * @return This stream, for purposes of chaining
         */
        message operator<<(send_msg_t);
		/**
         * @brief Finish and add row to message, argument indicates end of row
         * @return This stream, for purposes of chaining
         */
        message_builder& operator<<(add_row_t);
        //TODO Make action rows work with this
        /**
         * @brief Build message in stream
         * @param c Component to add
         * @return This stream, for purposes of chaining
         */
        message_builder& operator<<(const component &c);
        /**
         * @brief Build message in stream
         * @param e Embed to add
         * @return This stream, for purposes of chaining
         */
        message_builder& operator<<(const embed &e);
        /**
         * @brief Build message in stream
         * @param f Pair of filename and filecontent to add
         * @return This stream, for purposes of chaining
         */
        message_builder& operator<<(const file_info &f);
	protected:
		/**
		 * @brief Add row to message
		 * 
		 */
		void add_row();
		/**
		 * @brief Add component to current row
		 * 
		 * @param c Current row
		 */
		void add_component(const component& c);
		message msg = message(); /*!< Message being built*/
	private:
		int n_buttons = 0; /*!< Number of buttons in current action row*/
		int n_rows = 0; /*!< Number of action rows in current message*/
		component current_action_row = component(); /*!< Action row currently being built*/
        
};
class DPP_EXPORT base_stream {
    public:
       
        /**
         * @brief Put message in stream
         * @param msg Message to add
         * @return This stream, for purposes of chaining
         */
        base_stream& operator<<(const message &msg);
        virtual void send(message msg, command_completion_event_t callback = utility::log_error()) = 0;
        virtual message send_sync(message msg) = 0;
        
};
/**
 * @brief A stream wrapper to send messages
 */ 
class DPP_EXPORT dm_stream : public base_stream {
	public:
		/**
		 * @brief Constructor
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel User to whom messages will be sent
		 */
		dm_stream(cluster& bot_, const user& out_user); 
		/**
		 * @brief Constructor
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel_id User id to whom messages will be sent
		 */
		dm_stream(cluster& bot_, snowflake out_user_id_);
		/**
		 * @brief Send message with stream
		 * @param callback Function to call when the API call completes. On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
		 */
	
		void send(message msg, command_completion_event_t callback = utility::log_error()) override;
		/**
		 * @brief Send message synchronously
		 * @return Message sent
		 * @warning This function is a blocking (synchronous) call and should only be used from within a separate thread.
 		 * Avoid direct use of this function inside an event handler.
		 */
		message send_sync(message msg) override;
		
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
		 * @brief Constructor
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel Channel where messages will be sent
		 */
		channel_stream(cluster& bot_, const channel& out_channel); 
		/**
		 * @brief Constructor
		 * 
		 * @param bot Cluster that will send messages
		 * @param out_channel_id Channel id where messages will be sent
		 */
		channel_stream(cluster& bot_, snowflake out_channel_id_);
		/**
		 * @brief Send message with stream
		 * @param msg Message to send
		 * @param callback Function to call when the API call completes. On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
		 */
	
		void send(message msg, command_completion_event_t callback = utility::log_error()) override;
		/**
		 * @brief Send message synchronously
		 * @param msg Message to send
		 * @return Message sent
		 * @warning This function is a blocking (synchronous) call and should only be used from within a separate thread.
 		 * Avoid direct use of this function inside an event handler.
		 */
		message send_sync(message msg) override;
		
	private:	
		cluster* bot; /**< Cluster which sends messages */
		snowflake out_channel_id; /**< Channel id in which messages are sent */

};

}