#ifndef SABER_SABER_HPP
#define SABER_SABER_HPP

#include <saber/export.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <ekizu/http_client.hpp>
#include <ekizu/shard.hpp>
#include <saber/commands.hpp>

namespace saber {
struct Saber {
	SABER_EXPORT explicit Saber(std::string_view token);

	[[nodiscard]] ekizu::Snowflake bot_id() const { return m_bot_id; }
	[[nodiscard]] CommandLoader &commands() { return m_commands; }
	[[nodiscard]] const ekizu::HttpClient &http() const { return m_http; }
	[[nodiscard]] ekizu::LruCache<ekizu::Snowflake, ekizu::Message> &
	messages_cache() {
		return m_messages_cache;
	}
	[[nodiscard]] ekizu::LruCache<ekizu::Snowflake, ekizu::User> &
	users_cache() {
		return m_users_cache;
	}
	[[nodiscard]] ekizu::Snowflake owner_id() const { return m_owner_id; }
	[[nodiscard]] std::string_view prefix() const { return m_prefix; }

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
	ekizu::LruCache<ekizu::Snowflake, ekizu::Message> m_messages_cache{500};
	ekizu::Snowflake m_owner_id;
	std::string m_prefix{">"};
	ekizu::Shard m_shard;
	ekizu::CurrentUser m_user;
	ekizu::LruCache<ekizu::Snowflake, ekizu::User> m_users_cache{500};
};
}  // namespace saber

#endif	// SABER_SABER_HPP
