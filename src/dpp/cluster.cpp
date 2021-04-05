#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <spdlog/spdlog.h>
#include <chrono>

namespace dpp {

cluster::cluster(const std::string &_token, uint32_t _intents, uint32_t _shards, uint32_t _cluster_id, uint32_t _maxclusters, spdlog::logger* _log)
	: token(_token), intents(_intents), numshards(_shards), cluster_id(_cluster_id), maxclusters(_maxclusters), log(_log)
{
	rest = new request_queue(this);
}

cluster::~cluster()
{
	delete rest;
}

confirmation_callback_t::confirmation_callback_t(const std::string &_type, const confirmable_t& _value, const http_request_completion_t& _http) : type(_type), value(_value), http_info(_http)
{
	if (type == "confirmation") {
		confirmation newvalue = std::get<confirmation>(_value);
		newvalue.success = (http_info.status < 400);
		value = newvalue;
	}
}

void cluster::start() {
	/* Start up all shards */
	for (uint32_t s = 0; s < numshards; ++s) {
		/* Filter out shards that arent part of the current cluster, if the bot is clustered */
		if (s % maxclusters == cluster_id) {
			/* TODO: DiscordClient should spawn a thread in its Run() */
			this->shards[s] = new DiscordClient(this, s, numshards, token, intents, log);
			this->shards[s]->Run();
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	}
}

void cluster::post_rest(const std::string &endpoint, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback) {
	/* NOTE: This is not a memory leak! The request_queue will free the http_request once it reaches the end of its lifecycle */
	rest->post_request(new http_request(endpoint, parameters, [callback](const http_request_completion_t& rv) {
		json j;
		if (rv.error == h_success && !rv.body.empty()) {
			j = json::parse(rv.body);
		}
		if (callback) {
			callback(j, rv);
		}
	}, postdata, method));
}

void cluster::message_create(const message &m, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages", m_post, m.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::message_edit(const message &m, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id), m_patch, m.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/messages/" + std::to_string(message_id) + "/crosspost", m_post, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id) + "/reactions/" + reaction + "/@me", m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_own_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/@me", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_all_reactions(const struct message &m, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id) + "/reactions",  m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_get_reactions(const struct message &m, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
	std::string parameters;
	if (before) {
		parameters.append("&before=" + std::to_string(before));
	}
	if (after) {
		parameters.append("&after=" + std::to_string(after));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			user_map users;
			for (auto & curr_user : j) {
				users[SnowflakeNotNull(&curr_user, "id")] = user().fill_from_json(&curr_user);
			}
			callback(confirmation_callback_t("user_map", users, http));
		}
	});
}

void cluster::message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/messages/" + std::to_string(message_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/messages/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_bulk(const std::vector<snowflake>& message_ids, snowflake channel_id, command_completion_event_t callback) {
	json j;
	for (auto & m : message_ids) {
		j.push_back(std::to_string(m));
	}
	this->post_rest("/api/channels", std::to_string(channel_id) + "/messages/bulk-delete", m_delete, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_create(const class channel &c, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(c.guild_id) + "/channels", m_post, c.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_edit(const class channel &c, command_completion_event_t callback) {
	json j = c.build_json(true);
	auto p = j.find("position");
	if (p != j.end()) {
		j.erase(p);
	}
	this->post_rest("/api/channels", std::to_string(c.id), m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_get(snowflake c, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(c), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_typing(const class channel &c, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(c.id) + "/typing", m_post, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/pins/" + std::to_string(message_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/pins/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_edit_position(const class channel &c, command_completion_event_t callback) {
	json j({ {"id", c.id}, {"position", c.position}  });
	this->post_rest("/api/guilds", std::to_string(c.guild_id) + "/channels/" + std::to_string(c.id), m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_edit_permissions(const class channel &c, snowflake overwrite_id, uint32_t allow, uint32_t deny, bool member, command_completion_event_t callback) {
	json j({ {"allow", std::to_string(allow)}, {"deny", std::to_string(deny)}, {"type", member ? 1 : 0}  });
	this->post_rest("/api/channels", std::to_string(c.id) + "/permissions/" + std::to_string(overwrite_id), m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_follow_news(const class channel &c, snowflake target_channel_id, command_completion_event_t callback) {
	json j({ {"webhook_channel_id", target_channel_id} });
	this->post_rest("/api/channels", std::to_string(c.id) + "/followers", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_delete_permission(const class channel &c, snowflake overwrite_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(c.id) + "/permissions/" + std::to_string(overwrite_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::invite_get(const std::string &invitecode, command_completion_event_t callback) {
	this->post_rest("/api/invites", dpp::url_encode(invitecode) + "?with_counts=true", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_invites_get(const class channel &c, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(c.id) + "/invites", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		invite_map invites;
		for (auto & curr_invite : j) {
			invites[SnowflakeNotNull(&curr_invite, "id")] = invite().fill_from_json(&curr_invite);
		}
		if (callback) {
			callback(confirmation_callback_t("invite_map", invites, http));
		}
	});
}

void cluster::channel_invite_create(const class channel &c, const class invite &i, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(c.id) + "/invites", m_post, i.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

void cluster::pins_get(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/pins", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		message_map pins_messages;
		for (auto & curr_message : j) {
			pins_messages[SnowflakeNotNull(&curr_message, "id")] = message().fill_from_json(&curr_message);
		}
		if (callback) {
		callback(confirmation_callback_t("message_map", pins_messages, http));
		}
	});
}

void cluster::gdm_add(snowflake channel_id, snowflake user_id, const std::string &access_token, const std::string &nick, command_completion_event_t callback) {
	json params;
	params["access_token"] = access_token;
	params["nick"] = nick;
	this->post_rest("/api/channels", std::to_string(channel_id) + "/recipients/" + std::to_string(user_id), m_put, params.dump(), [callback](json &j, const http_request_completion_t& http) {
	if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::gdm_remove(snowflake channel_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/recipients/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::invite_delete(const std::string &invitecode, command_completion_event_t callback) {
	this->post_rest("/api/invites", dpp::url_encode(invitecode), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_delete(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(channel_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_create(const class guild &g, command_completion_event_t callback) {
	this->post_rest("/api/guilds", "", m_post, g.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_edit(const class guild &g, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(g.id), m_patch, g.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(&j), http));
		}
	});
}

void cluster::template_get(const std::string &code, command_completion_event_t callback) {
	this->post_rest("/api/guilds", "templates/" + code, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	this->post_rest("/api/guilds", "templates/" + code, m_post, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_templates_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/templates", m_get, "", [callback](json &j, const http_request_completion_t& http) {
	dtemplate_map dtemplates;
		for (auto & curr_dtemplate : j) {
			dtemplates[SnowflakeNotNull(&curr_dtemplate, "id")] = dtemplate().fill_from_json(&curr_dtemplate);
		}
		if (callback) {
				callback(confirmation_callback_t("dtemplate_map", dtemplates, http));
		}
	});
}

void cluster::guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	params["description"] = description;
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/templates", m_post, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/templates/" + code, m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	params["description"] = description;
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/templates/" + code, m_patch, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/templates/" + code, m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::user_get(snowflake user_id, command_completion_event_t callback) {
	this->post_rest("/api/users", std::to_string(user_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_get(command_completion_event_t callback) {
	this->post_rest("/api/users", "@me", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_get_guilds(command_completion_event_t callback) {
	this->post_rest("/api/users", "@me/guilds", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			guild_map guilds;
			for (auto & curr_guild : j) {
				guilds[SnowflakeNotNull(&curr_guild, "id")] = guild().fill_from_json(&curr_guild);
			}
			callback(confirmation_callback_t("guild_map", guilds, http));
		}
	});
}

void cluster::guild_delete(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::role_create(const class role &r, command_completion_event_t callback) {
	this->post_rest("/api/channels", std::to_string(r.guild_id) + "/roles", m_post, r.build_json(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}

void cluster::role_edit(const class role &r, command_completion_event_t callback) {
	json j = r.build_json(true);
	auto p = j.find("position");
	if (p != j.end()) {
		j.erase(p);
	}
	this->post_rest("/api/guilds", std::to_string(r.guild_id) + "/roles/" + std::to_string(r.id) , m_patch, j.dump(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}

void cluster::guild_emojis_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/emojis", m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			emoji_map emojis;
			for (auto & curr_emoji : j) {
				emojis[SnowflakeNotNull(&curr_emoji, "id")] = emoji().fill_from_json(&curr_emoji);
			}
			callback(confirmation_callback_t("emoji_map", emojis, http));
		}
	});
}

void cluster::guild_emoji_get(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/emojis/" + std::to_string(emoji_id), m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("emoji", emoji().fill_from_json(&j), http));
		}
	});
}


void cluster::roles_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/roles", m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			role_map roles;
			for (auto & curr_role : j) {
				roles[SnowflakeNotNull(&curr_role, "id")] = role().fill_from_json(guild_id, &curr_role);
			}
			callback(confirmation_callback_t("role_map", roles, http));
		}
	});
}

void cluster::channels_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/channels", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map channels;
			for (auto & curr_channel: j) {
				channels[SnowflakeNotNull(&curr_channel, "id")] = channel().fill_from_json(&curr_channel);
			}
			callback(confirmation_callback_t("channel_map", channels, http));
		}
	});
}


void cluster::messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
	std::string parameters;
	if (around) {
		parameters.append("&around=" + std::to_string(around));
	}
	if (before) {
		parameters.append("&before=" + std::to_string(before));
	}
	if (after) {
		parameters.append("&after=" + std::to_string(after));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest("/api/channels", std::to_string(channel_id) + "/messages" + parameters, m_get, json(), [channel_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			message_map messages;
			for (auto & curr_message : j) {
				messages[SnowflakeNotNull(&curr_message, "id")] = message().fill_from_json(&curr_message);
			}
			callback(confirmation_callback_t("message_map", messages, http));
		}
	});
}

void cluster::role_edit_position(const class role &r, command_completion_event_t callback) {
	json j({ {"id", r.id}, {"position", r.position}  });
	this->post_rest("/api/guilds", std::to_string(r.guild_id) + "/roles/" + std::to_string(r.id), m_patch, j.dump(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}

void cluster::role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/roles/" + std::to_string(role_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) 
			callback(confirmation_callback_t("confirmation", confirmation(), http));{
		}
	});
}

void cluster::on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update) {
	this->dispatch.voice_state_update = _voice_state_update; 
}

void cluster::on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create) {
	this->dispatch.interaction_create = _interaction_create; 
}

void cluster::on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete) {
	this->dispatch.guild_delete = _guild_delete; 
}

void cluster::on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete) {
	this->dispatch.channel_delete = _channel_delete; 
}

void cluster::on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update) {
	this->dispatch.channel_update = _channel_update; 
}

void cluster::on_ready (std::function<void(const ready_t& _event)> _ready) {
	this->dispatch.ready = _ready; 
}

void cluster::on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete) {
	this->dispatch.message_delete = _message_delete; 
}

void cluster::on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete) {
	this->dispatch.application_command_delete = _application_command_delete; 
}

void cluster::on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove) {
	this->dispatch.guild_member_remove = _guild_member_remove; 
}

void cluster::on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create) {
	this->dispatch.application_command_create = _application_command_create; 
}

void cluster::on_resumed (std::function<void(const resumed_t& _event)> _resumed) {
	this->dispatch.resumed = _resumed; 
}

void cluster::on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create) {
	this->dispatch.guild_role_create = _guild_role_create; 
}

void cluster::on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start) {
	this->dispatch.typing_start = _typing_start; 
}

void cluster::on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add) {
	this->dispatch.message_reaction_add = _message_reaction_add; 
}

void cluster::on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk) {
	this->dispatch.guild_members_chunk = _guild_members_chunk; 
}

void cluster::on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove) {
	this->dispatch.message_reaction_remove = _message_reaction_remove; 
}

void cluster::on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create) {
	this->dispatch.guild_create = _guild_create; 
}

void cluster::on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create) {
	this->dispatch.channel_create = _channel_create; 
}

void cluster::on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji) {
	this->dispatch.message_reaction_remove_emoji = _message_reaction_remove_emoji; 
}

void cluster::on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk) {
	this->dispatch.message_delete_bulk = _message_delete_bulk; 
}

void cluster::on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update) {
	this->dispatch.guild_role_update = _guild_role_update; 
}

void cluster::on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete) {
	this->dispatch.guild_role_delete = _guild_role_delete; 
}

void cluster::on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update) {
	this->dispatch.channel_pins_update = _channel_pins_update; 
}

void cluster::on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all) {
	this->dispatch.message_reaction_remove_all = _message_reaction_remove_all; 
}

void cluster::on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update) {
	this->dispatch.voice_server_update = _voice_server_update; 
}

void cluster::on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update) {
	this->dispatch.guild_emojis_update = _guild_emojis_update; 
}

void cluster::on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update) {
	this->dispatch.presence_update = _presence_update; 
}

void cluster::on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update) {
	this->dispatch.webhooks_update = _webhooks_update; 
}

void cluster::on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add) {
	this->dispatch.guild_member_add = _guild_member_add; 
}

void cluster::on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete) {
	this->dispatch.invite_delete = _invite_delete; 
}

void cluster::on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update) {
	this->dispatch.guild_update = _guild_update; 
}

void cluster::on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update) {
	this->dispatch.guild_integrations_update = _guild_integrations_update; 
}

void cluster::on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update) {
	this->dispatch.guild_member_update = _guild_member_update; 
}

void cluster::on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update) {
	this->dispatch.application_command_update = _application_command_update; 
}

void cluster::on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create) {
	this->dispatch.invite_create = _invite_create; 
}

void cluster::on_message_update (std::function<void(const message_update_t& _event)> _message_update) {
	this->dispatch.message_update = _message_update; 
}

void cluster::on_user_update (std::function<void(const user_update_t& _event)> _user_update) {
	this->dispatch.user_update = _user_update; 
}

void cluster::on_message_create (std::function<void(const message_create_t& _event)> _message_create) {
	this->dispatch.message_create = _message_create; 
}

void cluster::on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add) {
	this->dispatch.guild_ban_add = _guild_ban_add; 
}

void cluster::on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create) {
	this->dispatch.integration_create = _integration_create;
}

void cluster::on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update) {
	this->dispatch.integration_update = _integration_update;
}

void cluster::on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete) {
	this->dispatch.integration_delete = _integration_delete;
}


};
