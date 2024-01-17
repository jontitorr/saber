#ifndef SABER_SABER_HPP
#define SABER_SABER_HPP

#include <saber/export.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <ekizu/http_client.hpp>
#include <ekizu/lru_cache.hpp>
#include <ekizu/shard.hpp>
#include <ekizu/voice_connection.hpp>
#include <ekizu/voice_state.hpp>
#include <saber/commands.hpp>
#include <saber/config.hpp>
#include <saber/reddit.hpp>

namespace saber {
struct Saber {
	SABER_EXPORT explicit Saber(Config config);

	[[nodiscard]] ekizu::Snowflake bot_id() const { return m_bot_id; }
	[[nodiscard]] CommandLoader &commands() { return m_commands; }
	[[nodiscard]] const ekizu::HttpClient &http() const { return m_http; }
	[[nodiscard]] ekizu::SnowflakeLruCache<ekizu::Guild> &guilds() {
		return m_guild_cache;
	}
	[[nodiscard]] ekizu::SnowflakeLruCache<
		ekizu::SnowflakeLruCache<ekizu::GuildMember>> &
	members() {
		return m_guild_member_cache;
	}
	[[nodiscard]] ekizu::SnowflakeLruCache<ekizu::Message> &messages() {
		return m_message_cache;
	}
	[[nodiscard]] ekizu::SnowflakeLruCache<ekizu::User> &users() {
		return m_user_cache;
	}
	[[nodiscard]] ekizu::SnowflakeLruCache<
		ekizu::SnowflakeLruCache<ekizu::VoiceState>> &
	voice_states() {
		return m_voice_state_cache;
	}
	[[nodiscard]] const Config &config() const { return m_config; }
	[[nodiscard]] ekizu::Snowflake owner_id() const {
		return config().owner_id;
	}
	[[nodiscard]] std::string_view prefix() const { return config().prefix; }

	SABER_EXPORT Result<ekizu::Permissions> get_guild_permissions(
		ekizu::Snowflake guild_id, ekizu::Snowflake user_id);
	SABER_EXPORT Result<ekizu::VoiceConnectionConfig *> join_voice_channel(
		ekizu::Snowflake guild_id, ekizu::Snowflake channel_id,
		const boost::asio::yield_context &yield);
	SABER_EXPORT Result<> leave_voice_channel(
		ekizu::Snowflake guild_id, const boost::asio::yield_context &yield);

	SABER_EXPORT void run(const boost::asio::yield_context &yield);

	template <ekizu::LogLevel level = ekizu::LogLevel::Debug, typename... Args>
	void log(Args &&...args) {
		switch (level) {
			case ekizu::LogLevel::Debug:
				return m_logger->debug(std::forward<Args>(args)...);
			case ekizu::LogLevel::Info:
				return m_logger->info(std::forward<Args>(args)...);
			case ekizu::LogLevel::Warn:
				return m_logger->warn(std::forward<Args>(args)...);
			case ekizu::LogLevel::Error:
				return m_logger->error(std::forward<Args>(args)...);
			case ekizu::LogLevel::Critical:
				return m_logger->critical(std::forward<Args>(args)...);
			case ekizu::LogLevel::Trace:
				return m_logger->trace(std::forward<Args>(args)...);
		}
	}

   private:
	void handle_event(ekizu::Event ev, const boost::asio::yield_context &yield);

	ekizu::Snowflake m_bot_id;
	CommandLoader m_commands;
	ekizu::HttpClient m_http;
	std::optional<spdlog::logger> m_logger;
	ekizu::SnowflakeLruCache<ekizu::Guild> m_guild_cache{500};
	ekizu::SnowflakeLruCache<ekizu::SnowflakeLruCache<ekizu::GuildMember>>
		m_guild_member_cache{500};
	ekizu::LruCache<ekizu::Snowflake, ekizu::Message> m_message_cache{500};
	ekizu::Shard m_shard;
	ekizu::CurrentUser m_user;
	ekizu::SnowflakeLruCache<ekizu::User> m_user_cache{500};
	std::unordered_map<ekizu::Snowflake, ekizu::VoiceConnectionConfig>
		m_voice_configs{500};
	ekizu::SnowflakeLruCache<boost::asio::experimental::channel<void(
		boost::system::error_code, const ekizu::VoiceConnectionConfig *)>>
		m_voice_ready_channels{500};
	ekizu::SnowflakeLruCache<ekizu::SnowflakeLruCache<ekizu::VoiceState>>
		m_voice_state_cache{500};
	Config m_config;
};
}  // namespace saber

#endif	// SABER_SABER_HPP
