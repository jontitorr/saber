#ifndef SABER_SABER_HPP
#define SABER_SABER_HPP

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <ekizu/http_client.hpp>
#include <ekizu/shard.hpp>
#include <saber/commands.hpp>

namespace saber {
struct Saber {
	explicit Saber(std::string_view token);

	void run(const boost::asio::yield_context &yield);
	void handle_event(ekizu::Event ev, const boost::asio::yield_context &yield);

	ekizu::Snowflake bot_id;
	CommandLoader commands;
	ekizu::HttpClient http;
	std::shared_ptr<spdlog::logger> logger;
	ekizu::LruCache<ekizu::Snowflake, ekizu::Message> messages_cache{500};
	ekizu::Snowflake owner_id;
	std::string prefix{">"};
	ekizu::Shard shard;
	ekizu::CurrentUser user;
	ekizu::LruCache<ekizu::Snowflake, ekizu::User> users_cache{500};
};
}  // namespace saber

#endif	// SABER_SABER_HPP
