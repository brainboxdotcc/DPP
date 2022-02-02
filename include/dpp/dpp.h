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
#include <dpp/version.h>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <dpp/exception.h>
#include <dpp/snowflake.h>
#include <dpp/misc-enum.h>
#include <dpp/managed.h>
#include <dpp/utility.h>
#include <dpp/voicestate.h>
#include <dpp/role.h>
#include <dpp/user.h>
#include <dpp/channel.h>
#include <dpp/guild.h>
#include <dpp/invite.h>
#include <dpp/dtemplate.h>
#include <dpp/emoji.h>
#include <dpp/ban.h>
#include <dpp/prune.h>
#include <dpp/voiceregion.h>
#include <dpp/integration.h>
#include <dpp/webhook.h>
#include <dpp/presence.h>
#include <dpp/intents.h>
#include <dpp/message.h>
#include <dpp/appcommand.h>
#include <dpp/stage_instance.h>
#include <dpp/auditlog.h>
#include <dpp/application.h>
#include <dpp/scheduled_event.h>
#include <dpp/discordclient.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <dpp/cache.h>
#include <dpp/queues.h>
#include <dpp/commandhandler.h>

namespace dpp {

/**
 * @brief Represents a reaction.
 * Can be filled for use in a collector
 */
class collected_reaction : public managed {
public:
	/// Reacting user
	user react_user;
	/// Reacting guild
	guild* react_guild;
	/// Reacting guild member
	guild_member react_member;
	/// Reacting channel
	channel* react_channel;
	/// Reacted emoji
	emoji react_emoji;
};

/**
 * @brief Template type for base class of channel collector
 */
typedef dpp::collector<dpp::channel_create_t, dpp::channel> channel_collector_t;

/**
 * @brief Template type for base class of thread collector
 */
typedef dpp::collector<dpp::thread_create_t, dpp::thread> thread_collector_t;

/**
 * @brief Template type for base class of role collector
 */
typedef dpp::collector<dpp::guild_role_create_t, dpp::role> role_collector_t;

/**
 * @brief Template type for base class of scheduled event collector
 */
typedef dpp::collector<dpp::guild_scheduled_event_create_t, dpp::scheduled_event> scheduled_event_collector_t;

/**
 * @brief Template type for base class of message collector
 */
typedef dpp::collector<dpp::message_create_t, dpp::message> message_collector_t;

/**
 * @brief Template type for base class of message reaction collector
 */
typedef dpp::collector<dpp::message_reaction_add_t, dpp::collected_reaction> reaction_collector_t;

/**
 * @brief Message collector.
 * Collects messages during a set timeframe and returns them in a list via the completed() method.
 */
class message_collector : public message_collector_t {
public:
	/**
	 * @brief Construct a new message collector object
	 * 
	 * @param cl cluster to associate the collector with
	 * @param duration Duration of time to run the collector for in seconds
	 */
	message_collector(cluster* cl, uint64_t duration) : message_collector_t::collector(cl, duration, cl->on_message_create) { }

	/**
	 * @brief Return the completed collection
	 * 
	 * @param list items collected during the timeframe specified
	 */
	virtual void completed(const std::vector<dpp::message>& list) = 0;

	/**
	 * @brief Select and filter the items which are to appear in the list
	 * This is called every time a new event is fired, to filter the event and determine which
	 * of the items is sent to the list. Returning nullptr excludes the item from the list.
	 * 
	 * @param element element to filter
	 * @return Returned item to add to the list, or nullptr to skip adding this element
	 */
	virtual const dpp::message* filter(const dpp::message_create_t& element) { return &element.msg; }

	/**
	 * @brief Destroy the message collector object
	 */
	virtual ~message_collector() = default;
};

/**
 * @brief Reaction collector.
 * Collects message reactions during a set timeframe and returns them in a list via the completed() method.
 */
class reaction_collector : public reaction_collector_t {
	snowflake message_id;
	collected_reaction react;
public:
	/**
	 * @brief Construct a new reaction collector object
	 * 
	 * @param cl cluster to associate the collector with
	 * @param duration Duration of time to run the collector for in seconds
	 * @param msg_id Optional message ID. If specified, only collects reactions for the given message
	 */
	reaction_collector(cluster* cl, uint64_t duration, snowflake msg_id = 0) : reaction_collector_t::collector(cl, duration, cl->on_message_reaction_add), message_id(msg_id) { }

	/**
	 * @brief Return the completed collection
	 * 
	 * @param list items collected during the timeframe specified
	 */
	virtual void completed(const std::vector<dpp::collected_reaction>& list) = 0;

	/**
	 * @brief Select and filter the items which are to appear in the list
	 * This is called every time a new event is fired, to filter the event and determine which
	 * of the items is sent to the list. Returning nullptr excludes the item from the list.
	 * 
	 * @param element element to filter
	 * @return Returned item to add to the list, or nullptr to skip adding this element
	 */
	virtual const dpp::collected_reaction* filter(const dpp::message_reaction_add_t& element) {
		/* Capture reactions for given message ID only */
		if (message_id == 0 || element.message_id == message_id) {
			react.id = element.message_id;
			react.react_user = element.reacting_user;
			react.react_guild = element.reacting_guild;
			react.react_member = element.reacting_member;
			react.react_channel = element.reacting_channel;
			react.react_emoji = element.reacting_emoji;
			return &react;
		} else {
			return nullptr;
		}
	}

	/**
	 * @brief Destroy the reaction collector object
	 */
	virtual ~reaction_collector() = default;
};

/**
 * @brief Channel collector.
 * Collects channels during a set timeframe and returns them in a list via the completed() method.
 */
class channel_collector : public channel_collector_t {
public:
	/**
	 * @brief Construct a new channel collector object
	 * 
	 * @param cl cluster to associate the collector with
	 * @param duration Duration of time to run the collector for in seconds
	 */
	channel_collector(cluster* cl, uint64_t duration) : channel_collector_t::collector(cl, duration, cl->on_channel_create) { }

	/**
	 * @brief Return the completed collection
	 * 
	 * @param list items collected during the timeframe specified
	 */
	virtual void completed(const std::vector<dpp::channel>& list) = 0;

	/**
	 * @brief Select and filter the items which are to appear in the list
	 * This is called every time a new event is fired, to filter the event and determine which
	 * of the items is sent to the list. Returning nullptr excludes the item from the list.
	 * 
	 * @param element element to filter
	 * @return Returned item to add to the list, or nullptr to skip adding this element
	 */
	virtual const dpp::channel* filter(const dpp::channel_create_t& element) { return element.created; }

	/**
	 * @brief Destroy the channel collector object
	 */
	virtual ~channel_collector() = default;
};

/**
 * @brief Thread collector.
 * Collects threads during a set timeframe and returns them in a list via the completed() method.
 */
class thread_collector : public thread_collector_t {
public:
	/**
	 * @brief Construct a new thread collector object
	 * 
	 * @param cl cluster to associate the collector with
	 * @param duration Duration of time to run the collector for in seconds
	 */
	thread_collector(cluster* cl, uint64_t duration) : thread_collector_t::collector(cl, duration, cl->on_thread_create) { }

	/**
	 * @brief Return the completed collection
	 * 
	 * @param list items collected during the timeframe specified
	 */
	virtual void completed(const std::vector<dpp::thread>& list) = 0;

	/**
	 * @brief Select and filter the items which are to appear in the list
	 * This is called every time a new event is fired, to filter the event and determine which
	 * of the items is sent to the list. Returning nullptr excludes the item from the list.
	 * 
	 * @param element element to filter
	 * @return Returned item to add to the list, or nullptr to skip adding this element
	 */
	virtual const dpp::thread* filter(const dpp::thread_create_t& element) { return &element.created; }

	/**
	 * @brief Destroy the thread collector object
	 */
	virtual ~thread_collector() = default;
};

/**
 * @brief Role collector.
 * Collects guild roles during a set timeframe and returns them in a list via the completed() method.
 */
class role_collector : public role_collector_t {
public:
	/**
	 * @brief Construct a new role collector object
	 * 
	 * @param cl cluster to associate the collector with
	 * @param duration Duration of time to run the collector for in seconds
	 */
	role_collector(cluster* cl, uint64_t duration) : role_collector_t::collector(cl, duration, cl->on_guild_role_create) { }

	/**
	 * @brief Return the completed collection
	 * 
	 * @param list items collected during the timeframe specified
	 */
	virtual void completed(const std::vector<dpp::role>& list) = 0;

	/**
	 * @brief Select and filter the items which are to appear in the list
	 * This is called every time a new event is fired, to filter the event and determine which
	 * of the items is sent to the list. Returning nullptr excludes the item from the list.
	 * 
	 * @param element element to filter
	 * @return Returned item to add to the list, or nullptr to skip adding this element
	 */
	virtual const dpp::role* filter(const dpp::guild_role_create_t& element) { return element.created; }

	/**
	 * @brief Destroy the role collector object
	 */
	virtual ~role_collector() = default;
};

/**
 * @brief Scheduled event collector.
 * Collects messages during a set timeframe and returns them in a list via the completed() method.
 */
class scheduled_event_collector : public scheduled_event_collector_t {
public:
	/**
	 * @brief Construct a new scheduled event collector object
	 * 
	 * @param cl cluster to associate the collector with
	 * @param duration Duration of time to run the collector for in seconds
	 */
	scheduled_event_collector(cluster* cl, uint64_t duration) : scheduled_event_collector_t::collector(cl, duration, cl->on_guild_scheduled_event_create) { }

	/**
	 * @brief Return the completed collection
	 * 
	 * @param list items collected during the timeframe specified
	 */
	virtual void completed(const std::vector<dpp::scheduled_event>& list) = 0;

	/**
	 * @brief Select and filter the items which are to appear in the list
	 * This is called every time a new event is fired, to filter the event and determine which
	 * of the items is sent to the list. Returning nullptr excludes the item from the list.
	 * 
	 * @param element element to filter
	 * @return Returned item to add to the list, or nullptr to skip adding this element
	 */
	virtual const dpp::scheduled_event* filter(const dpp::guild_scheduled_event_create_t& element) { return &element.created; }

	/**
	 * @brief Destroy the scheduled event collector object
	 */
	virtual ~scheduled_event_collector() = default;
};

};