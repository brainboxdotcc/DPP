#include <dpp/discord.h>

namespace dpp {

guild::guild() :
        id(0),
        flags(0),
        owner_id(0),
        voice_region(r_us_central),
        afk_channel_id(0),
        afk_timeout(0),
        widget_channel_id(0),
        verification_level(0),
        default_message_notifications(0),
        explicit_content_filter(0),
        mfa_level(0),
        application_id(0),
        system_channel_id(0),
        rules_channel_id(0),
        joined_at(0),
        member_count(0),
        premium_tier(0),
        premium_subscription_count(0),
        public_updates_channel_id(0),
        max_video_channel_users(0)

{
}

guild::~guild()
{
}

guild_member::guild_member()
{
}

guild_member::~guild_member()
{
}

bool guild::is_large() {
	return this->flags & g_large;
}

bool guild::is_unavailable() {
	return this->flags & g_unavailable;
}

bool guild::widget_enabled() {
	return this->flags & g_widget_enabled;
}

bool guild::has_invite_splash() {
	return this->flags & g_invite_splash;
}

bool guild::has_vip_regions() {
	return this->flags & g_vip_regions;
}

bool guild::has_vanity_url() {
	return this->flags & g_vanity_url;
}

bool guild::is_verified() {
	return this->flags & g_verified;
}

bool guild::is_partnered() {
	return this->flags & g_partnered;
}

bool guild::is_community() {
	return this->flags & g_community;
}

bool guild::has_commerce() {
	return this->flags & g_commerce;
}

bool guild::has_news() {
	return this->flags & g_news;
}

bool guild::is_discoverable() {
	return this->flags & g_discoverable;
}

bool guild::is_featureable() {
	return this->flags & g_featureable;
}

bool guild::has_animated_icon() {
	return this->flags & g_animated_icon;
}

bool guild::has_banner() {
	return this->flags & g_banner;
}

bool guild::is_welcome_screen_enabled() {
	return this->flags & g_welcome_screen_enabled;
}

bool guild::has_member_verification_gate() {
	return this->flags & g_member_verification_gate;
}

bool guild::is_preview_enabled() {
	return this->flags & g_preview_enabled;
}

};

